#pragma once

#include "basejob.h"

#include <QtCore/QFuture>

namespace Quotient {

template <typename FnT, typename JobT>
concept BoundResultHandler = std::invocable<FnT, JobT*> || std::invocable<FnT>
                             || requires(FnT f, JobT j) { f(collectResponse(&j)); };

template <typename FnT, typename JobT>
concept ResultHandler = BoundResultHandler<FnT, JobT> || std::is_member_function_pointer_v<FnT>;

//! \brief A job pointer and a QFuture in a single package
//!
//! This class wraps a pointer to any job the same way QPointer does: it turns to nullptr when
//! the job is destroyed. On top of that though, it provides you with an interface of QFuture as-if
//! obtained by calling `QtFuture::connect(job, &BaseJob::result).then([job] { return job; });`
//! before any other slot is connected to it. In the end, you (still) get the interface of \p JobT
//! at `handle->`, and `handle.` gives you the interface (very close to that, read below for
//! differences) of `QFuture<JobT*>`.
//!
//! You can mix usage of the two interfaces, bearing in mind that any continuation attached via
//! the future interface will overtake anything connected to `BaseJob::result` but come behind
//! anything connected to `BaseJob::finished` (that applies to `onCanceled()`, too).
//!
//! QFuture is somewhat rigid in terms of what it accepts for (normal, i.e. not cancelled)
//! continuations: the continuation function must accept a single argument of the type carried by
//! the future, or of the future type itself. JobHandle allows normal continuation functions (i.e.
//! those passed to `then()`, `onResult()` and `onFailure()`) to accept:
//! - no parameters;
//! - `JobT*` or any pointer it is convertible to (`const JobT*`, `BaseJob*` etc.);
//! - the value returned by calling `collectResponse()` with the above pointer as the parameter.
//!
//! Aside from free functions and function objects (including lambdas), you can also pass member
//! functions of QObject-derived classes (`connect()` slot style) to all continuations, including
//! onCanceled().
//!
//! JobHandle doesn't support passing its full type to continuation functions like QFuture does,
//! as there's no case for that (we don't need to deal with exceptions).
//!
//! This extended interface helps with migration of the current code that `connect()`s to the job
//! completion signals. The existing code will (mostly) run fine without changes; the only thing
//! that will stop working is using `auto*` for a variable initialised from  `Connection::callApi`
//! (plain `auto` still works). If you want to migrate the existing code to the future-like
//! interface, just replace:
//! \code
//! auto j = callApi<Job>(jobParams...);
//! connect(j, &BaseJob::result, object, slot);
//! \endcode
//! with `callApi<Job>(jobParams...).onResult(object, slot);` - that's all. If you have a connection
//! to `BaseJob::success`, use `then()` instead of `onResult()`, and if you only connect to
//! `BaseJob::failure`, `onFailure()` is at your service. And you can also combine the two using
//! `then()`, e.g.:
//! \code
//! callApi<Job>(jobParams...).then([this] { /* on success... */ },
//!                                 [this] { /* on failure... */ });
//! \endcode
//!
//! One more extension to QFuture is the way the returned value is treated:
//! - if your function returns `void` the continuation will have type `JobHandler<JobT>` and carry
//!    the same pointer as before;
//! - if your function returns a `JobHandle` (e.g. from another call to `Connection::callApi`),
//!   it will be automatically rewrapped into a `QFuture`, because `QFuture<JobHandle<AnotherJobT>>`
//!   is rather unwieldy for any intents and purposes, and `JobHandle<AnotherJobT>` would have
//!   a very weird QPointer interface as that new job doesn't even exist when continuation is
//!   constructed;
//! - otherwise, the return value is wrapped in a "normal" QFuture, JobHandle waves you good-bye and
//!   further continuations will follow pristine QFuture rules.
template <class JobT>
class JobHandle : public QPointer<JobT>, public QFuture<JobT*> {
public:
    using pointer_type = QPointer<JobT>;
    using future_value_type = JobT*;
    using future_type = QFuture<future_value_type>;

private:
    //! A placeholder structure with a private type, co-sitting as a no-op function object
    struct Skip : public decltype([](future_value_type) {}) {};

    JobHandle(JobT* job, future_type&& futureToWrap)
        : pointer_type(job), future_type(std::move(futureToWrap))
    {}

    static future_type setupFuture(JobT* job)
    {
        return job ? job->future().then([job] { return future_value_type{ job }; }) : future_type{};
    }

public:
    QUO_IMPLICIT JobHandle(JobT* job = nullptr) : JobHandle(job, setupFuture(job)) {}

    //! \brief Attach a continuation to a successful or unsuccessful completion of the future
    //!
    //! The continuation passed via \p fn should be an invokable that accepts one of the following:
    //! 1) no arguments - this is meant to simplify transition from job completion handlers
    //!    connect()ed to BaseJob::result, BaseJob::success or BaseJob::failure.
    //! 2) a pointer to a (const, if you want) job object - this can be either `BaseJob*`,
    //!    `JobT*` (recommended), or anything in between. Unlike slot functions connected
    //!    to BaseJob signals, this option allows you to access the specific job type so you don't
    //!    need to carry the original job pointer in a lambda - JobHandle does it for you. This is
    //!    meant to be a transitional form on the way to (3); eventually we should migrate to
    //!    (1)+(3) entirely.
    //! 3) the type returned by `collectResponse()` if it's well-formed (it is for all generated
    //!    jobs, needs overloading for manual jobs).
    //!
    //! \note The continuation returned from onResult() will not be triggered if/when the future is
    //!       cancelled or the underlying job is abandoned; use onCanceled() to catch cancellations.
    //!       You can also connect to BaseJob::finished using the QPointer interface of the handle
    //!       if you need to do something before _any_ continuation attached to his job kicks in.
    //!
    //! \param config passed directly to QFuture::then() as the first argument (see
    //!               the documentation on QFuture::then() for accepted types) and can also be used
    //!               as the object for a slot-like member function in QObject::connect() fashion
    //! \param fn the continuation function to attach to the future; can be a member function
    //!           if \p config is a pointer to an QObject-derived class
    //! \return if \p fn returns `void`, a new JobHandle for the same job, with the continuation
    //!         attached; otherwise, the return value of \p fn wrapped in a plain QFuture
    template <typename ConfigT, ResultHandler<JobT> FnT>
    auto onResult(ConfigT config, FnT&& fn)
    {
        return rewrap(future_type::then(config, continuation(std::forward<FnT>(fn), config)));
    }

    //! The overload for onResult matching 1-arg QFuture::then
    template <BoundResultHandler<JobT> FnT>
    auto onResult(FnT&& fn)
    {
        return rewrap(future_type::then(continuation(std::forward<FnT>(fn))));
    }

    //! \brief Attach continuations depending on the job success or failure
    //!
    //! This is inspired by `then()` in JavaScript; beyond the first argument passed through to
    //! `QFuture::then`, it accepts two more arguments (\p onFailure is optional), combining them
    //! in a single continuation: if the job ends with success, \p onSuccess is called; if the job
    //! fails, \p onFailure is called. The requirements to both functions are the same as to the
    //! single function passed to onResult().
    template <typename ConfigT, ResultHandler<JobT> SuccessFnT, ResultHandler<JobT> FailureFnT = Skip>
    auto then(ConfigT config, SuccessFnT&& onSuccess, FailureFnT&& onFailure = {})
        requires requires(future_type f) { f.then(config, Skip{}); }
    {
        return rewrap(future_type::then(
            config, combineContinuations(std::forward<SuccessFnT>(onSuccess),
                                         std::forward<FailureFnT>(onFailure), config)));
    }

    //! The overload making the combined continuation as if with 1-arg QFuture::then
    template <BoundResultHandler<JobT> SuccessFnT, BoundResultHandler<JobT> FailureFnT = Skip>
    auto then(SuccessFnT&& onSuccess, FailureFnT&& onFailure = {})
    {
        return rewrap(future_type::then(combineContinuations(std::forward<SuccessFnT>(onSuccess),
                                                             std::forward<FailureFnT>(onFailure))));
    }

    //! Same as then(config, [] {}, fn)
    template <typename FnT>
    auto onFailure(auto config, FnT&& fn)
    {
        return then(config, Skip{}, std::forward<FnT>(fn));
    }

    //! Same as then([] {}, fn)
    template <typename FnT>
    auto onFailure(FnT&& fn)
    {
        return then(Skip{}, std::forward<FnT>(fn));
    }

    //! Same as QFuture::onCanceled but accepts QObject-derived member functions and rewraps
    //! returned values
    template <typename FnT>
    auto onCanceled(QObject* context, FnT&& fn)
    {
        return rewrap(
            future_type::onCanceled(context, bindToContext(std::forward<FnT>(fn), context)));
    }

    //! Same as QFuture::onCanceled but accepts QObject-derived member functions and rewraps
    //! returned values
    template <typename FnT>
    auto onCanceled(FnT&& fn)
    {
        return rewrap(future_type::onCanceled(BoundFn{ std::forward<FnT>(fn) }));
    }

    //! Get a QFuture for the value returned by `collectResponse()` called on the underlying job
    auto responseFuture()
    {
        return future_type::then([](auto* j) { return collectResponse(j); });
    }

    //! \brief Abandon the underlying job, if there's one pending
    //!
    //! Unlike cancel() that only applies to the current future object but not the upstream chain,
    //! this actually goes up to the job and calls abandon() on it, thereby cancelling the entire
    //! chain of futures attached to it.
    //! \sa BaseJob::abandon
    void abandon()
    {
        if (auto pJob = pointer_type::get(); isJobPending(pJob)) {
            Q_ASSERT(QThread::currentThread() == pJob->thread());
            pJob->abandon(); // Triggers cancellation of the future
        }
    }

private:
    //! A function object that can be passed to QFuture::then and QFuture::onCanceled
    template <typename FnT>
    struct BoundFn {
        auto operator()() { return callFn<false>(nullptr); } // For QFuture::onCanceled
        auto operator()(future_value_type job) { return callFn(job); } // For QFuture::then

        template <bool AllowJobArg = true>
        auto callFn(future_value_type job)
        {
            if constexpr (std::invocable<FnT>) {
                return std::forward<FnT>(fn)();
            } else {
                static_assert(AllowJobArg, "onCanceled continuations should not accept arguments");
                if constexpr (requires { fn(job); })
                    return fn(job);
                else if constexpr (requires { collectResponse(job); }) {
                    static_assert(
                        requires { fn(collectResponse(job)); },
                        "The continuation function must accept either of: 1) no arguments; "
                        "2) the job pointer itself; 3) the value returned by collectResponse(job)");
                    return fn(collectResponse(job));
                }
            }
        }

        // See https://www.cppstories.com/2021/no-unique-address/
#ifndef Q_CC_CLANG
        // Apple Clang crashes with ICE and vanilla Clang 17 generates faulty code if fn has no
        // unique address. https://github.com/llvm/llvm-project/issues/59831 might be related.
        [[no_unique_address]]
#endif
        FnT fn;
    };

    template <typename FnT>
    BoundFn(FnT&&) -> BoundFn<FnT>;

    template <typename FnT, typename ConfigT = Skip>
    static auto bindToContext(FnT&& fn, ConfigT config = {})
    {
        // Even though QFuture::then() and QFuture::onCanceled() can use context QObjects
        // to determine the execution thread, they cannot bind slots to these context objects,
        // the way QObject::connect() does; so we do it here.
        if constexpr (std::derived_from<std::remove_pointer_t<ConfigT>, QObject>
                      && std::is_member_function_pointer_v<FnT>) {
            return BoundFn{ std::bind_front(std::forward<FnT>(fn), config) };
        } else
            return BoundFn{ std::forward<FnT>(fn) };
    }

    template <ResultHandler<JobT> FnT, typename ConfigT = Skip>
    static auto continuation(FnT&& fn, ConfigT config = {})
    {
        return [f = bindToContext(std::forward<FnT>(fn), config)](future_value_type arg) mutable {
            if constexpr (std::is_void_v<decltype(f(arg))>) {
                f(arg);
                return arg;
            } else
                return f(arg);
        };
    }

    template <ResultHandler<JobT> SuccessFnT, ResultHandler<JobT> FailureFnT, typename ConfigT = Skip>
    static auto combineContinuations(SuccessFnT&& onSuccess, FailureFnT&& onFailure,
                                     ConfigT config = {})
    {
        return [sFn = bindToContext(std::forward<SuccessFnT>(onSuccess), config),
                fFn = bindToContext(std::forward<FailureFnT>(onFailure), config)](
                   future_value_type job) mutable {
            using sType = decltype(sFn(job));
            using fType = decltype(fFn(job));
            if constexpr (std::is_void_v<sType> && std::is_void_v<fType>)
                return (job->status().good() ? sFn(job) : fFn(job), job);
            else if constexpr (std::is_same_v<FailureFnT, Skip>) {
                // Still call fFn to suppress unused lambda warning
                return job->status().good() ? sFn(job) : (fFn(job), sType{});
            } else
                return job->status().good() ? sFn(job) : fFn(job);
        };
    }

    auto rewrap(future_type&& ft) const
    {
        return JobHandle(pointer_type::get(), std::move(ft));
    }

    template <typename NewJobT>
    auto rewrap(QFuture<JobHandle<NewJobT>> ft)
        -> QFuture<typename JobHandle<NewJobT>::future_value_type>
    {
        // When a continuation function returns a job handle (e.g. by invoking callApi() inside of
        // it) that handle ends up being wrapped in a QFuture by QFuture::then() or
        // QFuture::onCanceled() called internally from their JobHandle counterparts. In a pure
        // QFuture world, there's QFuture::unwrap() to flatten the nested futures; unfortunately,
        // QFuture::unwrap() requires the nested object to be exactly a QFuture, not anything
        // derived from it. This method basically does what QFuture::unwrap() does, but is much
        // simpler because we don't need to deal with exceptions and already know the nested type.
        // It still returns QFuture and not JobHandle. Were it a JobHandle, its QPointer interface
        // would be rather confusing: initially it would be nullptr because the job doesn't even
        // exist when the continuation is constructed, and only later it would change its value
        // to something useful. Unless the client code stored the original JobHandle, it would
        // lose that change and only store nullptr; and if it stores a JobHandle then it can just
        // use the QFuture interface instead. Therefore a pure QFuture is returned instead, that
        // settles when the underlying job finishes or gets cancelled.
        QFutureInterface<typename JobHandle<NewJobT>::future_value_type> newPromise(
            QFutureInterfaceBase::State::Pending);
        ft.then([newPromise](JobHandle<NewJobT> nestedHandle) mutable {
            Q_ASSERT(nestedHandle.isStarted());
            newPromise.reportStarted();
            nestedHandle.then([newPromise]() mutable { newPromise.reportFinished(); })
                .onCanceled([newPromise]() mutable { newPromise.cancelAndFinish(); });
          }).onCanceled([newPromise]() mutable {
            newPromise.reportStarted();
            newPromise.cancelAndFinish();
        });
        return newPromise.future();
    }

    static auto rewrap(auto someOtherFuture) { return someOtherFuture; }
};

template <std::derived_from<BaseJob> JobT>
JobHandle(JobT*) -> JobHandle<JobT>;

} // namespace Quotient

Q_DECLARE_SMART_POINTER_METATYPE(Quotient::JobHandle)
