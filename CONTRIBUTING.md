# Contributing

Feedback and contributions are very welcome!

Here's help on how to make contributions, divided into the following sections:

The quick-read part:
* general information,
* vulnerability reporting,
* documentation.

The long-read part:
* code changes,
* how to check proposed changes before submitting them,
* reuse of other libraries, frameworks etc.

## General information

For specific proposals, please provide them as
[pull requests](https://github.com/quotient-im/libQuotient/pulls)
or
[issues](https://github.com/quotient-im/libQuotient/issues)
For general discussion, feel free to use our Matrix room:
[#quotient:matrix.org](https://matrix.to/#/#quotient:matrix.org).

If you're new to the project (or FLOSS in general),
[issues tagged as easy](https://github.com/quotient-im/libQuotient/labels/easy)
are smaller tasks that may typically take 1-3 days.
You are welcome aboard!

### Pull requests and different branches recommended

Pull requests are preferred, since they are specific.
See the GitHub Help [articles about pull requests](https://help.github.com/articles/using-pull-requests/)
to learn how to deal with them.

We recommend creating different branches for different (logical)
changes, and creating a pull request when you're done into the master branch.
See the GitHub documentation on
[creating branches](https://help.github.com/articles/creating-and-deleting-branches-within-your-repository/)
and
[using pull requests](https://help.github.com/articles/using-pull-requests/).

### How we handle proposals

We use GitHub to track all changes via its
[issue tracker](https://github.com/quotient-im/libQuotient/issues) and
[pull requests](https://github.com/quotient-im/libQuotient/pulls).
Specific changes are proposed using those mechanisms.
Issues are assigned to an individual who works on it and then marks it complete.
If there are questions or objections, the conversation area of that
issue or pull request is used to resolve it.

<!--
### Developer Certificate of Origin (DCO) - not enforced yet

All contributions (including pull requests) must agree to
the [Developer Certificate of Origin (DCO) version 1.1](doc/dco.txt).
This is exactly the same one created and used by the Linux kernel developers
and posted on <http://developercertificate.org/>.
This is a developer's certification that he or she has the right to
submit the patch for inclusion into the project.

Simply submitting a contribution implies this agreement, however,
please include a "Signed-off-by" tag in every patch
(this tag is a conventional way to confirm that you agree to the DCO).
You can do this with <tt>git commit --signoff</tt> (the <tt>-s</tt> flag
is a synonym for <tt>--signoff</tt>).

Another way to do this is to write the following at the end of the commit
message, on a line by itself separated by a blank line from the body of
the commit:

    Signed-off-by: YOUR NAME <YOUR.EMAIL@EXAMPLE.COM>

You can signoff by default in this project by creating a file
(say "git-template") that contains
some blank lines and the signed-off-by text above;
then configure git to use that as a commit template.  For example:

    git config commit.template ~/cii-best-practices-badge/git-template

It's not practical to fix old contributions in git, so if one is forgotten,
do not try to fix them.  We presume that if someone sometimes used a DCO,
a commit without a DCO is an accident and the DCO still applies.
-->
### License

Unless a contributor explicitly specifies otherwise, we assume contributors
to agree that all contributed code is released either under *LGPL v2.1 or later*.
This is more than just [LGPL v2.1 libQuotient now uses](./COPYING)
because the project plans to switch to LGPL v3 for library code in the near future.
<!-- The below is invalid yet!
All new contributed material that is not executable, including all text when not executed, is also released under the
[Creative Commons Attribution 4.0 International (CC BY 4.0) license](https://creativecommons.org/licenses/by/4.0/) or later.
-->

Any components proposed for reuse should have a license that permits releasing
a derivative work under *LGPL v2.1 or later* or LGPL v3. Moreover, the license of
a proposed component should be approved by OSI, no exceptions.

## Vulnerability reporting (security issues) - see [SECURITY.md](./SECURITY.md)

## Documentation changes

Most of the documentation is in Markdown format. All Markdown files use the .md
filename extension. Any help on fixing/extending these is more than welcome.

Where reasonable, limit yourself to Markdown that will be accepted by different
markdown processors (e.g., what is specified by CommonMark or the original
Markdown). In practice, as long as libQuotient is hosted at GitHub,
[GFM (GitHub-flavoured Markdown)](https://help.github.com/articles/github-flavored-markdown/)
is used to show those files in a browser, so it's fine to use its extensions.
In particular, you can mark code snippets with the programming language used;
blank lines separate paragraphs, newlines inside a paragraph do *not* force a line break.

Beware - this is *not* the same markdown algorithm used by GitHub when it
renders issue or pull comments; in those cases
[newlines in paragraph-like content are considered as real line breaks](https://help.github.com/articles/writing-on-github/);
unfortunately this other algorithm is *also* called GitHub-flavoured markdown.
(Yes, it'd be better if there were different names for different things.)

In your markdown, please don't use tab characters and avoid "bare" URLs.
In a hyperlink, the link text and URL should be on the same line.
While historically we didn't care about the line length in markdown texts
(and more often than not put the whole paragraph into one line), this is subject
to change anytime soon, with 80-character limit _recommendation_
(which is softer than the limit for C/C++ code) imposed on everything
_except hyperlinks_ (because wrapping hyperlinks breaks the rendering).

Do not use trailing two spaces for line breaks, since these cannot be seen
and may be silently removed by some tools. If, for whatever reason, a blank line
is not an option, use <tt>&lt;br&nbsp;/&gt;</tt> (an HTML break).

## End of TL;DR

If you don't plan/have substantial contributions, you can end reading here. Further sections are for those who's going to actively hack on the library code.

## Code changes

The code should strive to be DRY (don't repeat yourself), clear, and obviously
correct (i.e. buildable). Some technical debt is inevitable,
just don't bankrupt us with it. Refactoring is welcome.

### Generated C++ code for CS API
The code in lib/csapi, lib/identity and lib/application-service, although
it resides in Git, is actually generated from (a soft fork of) the official
Matrix Swagger/OpenAPI definition files. Do not edit C++ files
in these directories by hand!

Now, if you're unhappy with something in there and want to improve the code,
you have to understand the way these files are produced and setup
some additional tooling. The shortest possible procedure resembling
the below text can be found in .travis.yml (our Travis CI configuration
actually regenerates those files upon every build). As described below,
there is a handy build target for CMake; patches with a similar target
for qmake are (you guessed it) very welcome.

#### Why generate the code at all?
Because before both original authors of libQuotient had to do monkey business of writing boilerplate code, with the same patterns, types etc., literally, for every single API endpoint, and one of the authors got fed up with it at some point in time. By then about 15 job classes were written; the entire API counts about 100 endpoints. Besides, the existing jobs had to be updated according to changes in CS API that have been, and will keep, coming. Other considerations can be found in [this talk about API description languages that briefly touches on GTAD](https://youtu.be/W5TmRozH-rg).

#### Prerequisites for CS API code generation
1. Get the source code of GTAD and its dependencies, e.g. using the command:
   `git clone --recursive https://github.com/KitsuneRal/gtad.git`
2. Build GTAD: in the source code directory, do `cmake . && cmake --build .`
   (you might need to pass `-DCMAKE_PREFIX_PATH=<path to Qt>`,
   similar to libQuotient itself).
3. Get the Matrix CS API definitions that are included in the matrix-doc repo:
   `git clone https://github.com/quotient-im/matrix-doc.git`
   (quotient-im/matrix-doc is a fork that's known to produce working code;
   you may want to use your own fork if you wish to alter something in the API).
4. If you plan to submit a PR or just would like the generated code to be
   formatted, you should either ensure you have clang-format (version 6 at least)
   in your PATH or pass the _absolute_ path to it by adding
   `-DCLANG_FORMAT=<absolute path>` to the CMake invocation below.

#### Generating CS API contents
1. Pass additional configuration to CMake when configuring libQuotient:
   `-DMATRIX_DOC_PATH=<path you your matrix-doc repo> -DGTAD_PATH=<path to gtad binary (not the repo!)>`.
   If everything's right, these two CMake variables will be mentioned in
   CMake output and will trigger configuration of an additional build target,
   see the next step.
2. Generate the code: `cmake --build <your build dir> --target update-api`;
   if you use CMake with GNU Make, you can just do `make update-api` instead.
   Building this target will create (overwriting without warning) `.h` and `.cpp`
   files in `lib/csapi`, `lib/identity`, `lib/application-service` for all
   YAML files it can find in `matrix-doc/api/client-server` and other files
   in `matrix-doc/api` these depend on.
3. Re-run CMake so that the build system knows about new files, if there are any.

#### Changing generated code
See the more detailed description of what GTAD is and how it works in the documentation on GTAD in its source repo. Only parts specific for libQuotient are described here.

GTAD uses the following three kinds of sources:
1. OpenAPI files. Each file is treated as a separate source (because this is how GTAD works now).
2. A configuration file, in our case it's lib/csapi/gtad.yaml - this one is common for the whole API.
3. Source code template files: lib/csapi/{{base}}.*.mustache - also common.

The mustache files have a templated (not in C++ sense) definition of a network
job, deriving from BaseJob; each job class is prepended, if necessary, with
data structure definitions used by this job. The look of those files is hideous
for a newcomer; and the only known highlighter that can handle the combination
of Mustache (originally a web templating language) and C++ is provided in CLion.
To slightly simplify things some more or less generic constructs are defined
in gtad.yaml (see its "mustache:" section). Adventurous souls that would like
to figure what's going on in these files should speak up in the Quotient room -
I (Kitsune) will be very glad to help you out.

The types map in `gtad.yaml` is the central switchboard when it comes to matching OpenAPI types with C++ (and Qt) ones. It uses the following type attributes aside from pretty obvious "imports:":
* `avoidCopy` - this attribute defines whether a const ref should be used instead of a value. For basic types like int this is obviously unnecessary; but compound types like `QVector` should rather be taken by reference when possible.
* `moveOnly` - some types are not copyable at all and must be moved instead (an obvious example is anything "tainted" with a member of type `std::unique_ptr<>`). The template will use `T&&` instead of `T` or `const T&` to pass such types around.
* `useOmittable` - wrap types that have no value with "null" semantics (i.e. number types and custom-defined data structures) into a special `Omittable<>` template defined in `converters.h` - a substitute for `std::optional` from C++17 (we're still at C++14 yet).
* `omittedValue` - an alternative for `useOmittable`, just provide a value used for an omitted parameter. This is used for bool parameters which normally are considered false if omitted (or they have an explicit default value, passed in the "official" GTAD's `defaultValue` variable).
* `initializer` - this is a _partial_ (see GTAD and Mustache documentation for explanations but basically it's a variable that is a Mustache template itself) that specifies how exactly a default value should be passed to the parameter. E.g., the default value for a `QString` parameter is enclosed into `QStringLiteral`.

Instead of relying on the event structure definition in the OpenAPI files, `gtad.yaml` uses pointers to libQuotient's event structures: `EventPtr`, `RoomEventPtr` and `StateEventPtr`. Respectively, arrays of events, when encountered in OpenAPI definitions, are converted to `Events`, `RoomEvents` and `StateEvents` containers. When there's no way to figure the type from the definition, an opaque `QJsonObject` is used, leaving the conversion to the library and/or client code.

### Comments

Whenever you add a new call to the library API that you expect to be used
from client code, you must supply a proper doc-comment along with the call.
Doxygen style is preferred; but JavaDoc is acceptable too. Some parts are
not documented at all; adding doc-comments to them is highly encouraged.

Doc-comments for summaries should be separate from those details. Either of
the two following ways is fine, with considerable preference on the first one:
1. Use `///` for the summary comment and `/*! ... */` for details.
2. Use `\brief` (or `@brief`) for the summary, and follow with details after
   an empty doc-comment line. You can use either of the delimiters in that case.

In the code, the advice for commenting is as follows:
* Don't restate what's happening in the code unless it's not really obvious.
  We assume the readers to have at least some command of C++ and Qt. If your
  code is not obvious, consider rewriting it for clarity.
* Both C++ and Qt still come with their arcane features and dark corners,
  and we don't want to limit anybody who'd feels they have a case for
  variable templates, raw literals, or use `qAsConst` to avoid container
  detachment. Use your experience to figure what might be less well-known to
  readers and comment such cases (references to web pages, Quotient wiki etc.
  are very much ok).
* Make sure to document not so much "what" but more "why" certain code is done
  the way it is. In the worst case, the logic of the code can be
  reverse-engineered; you rarely can reverse-engineer the line of reasoning and
  the pitfalls avoided.

### API conventions

Calls, data structures and other symbols not intended for use by clients
should _not_ be exposed in (public) .h files, unless they are necessary
to declare other public symbols. In particular, this involves private members
(functions, typedefs, or variables) in public classes; use pimpl idiom to hide
implementation details as much as possible. `_impl` namespace is reserved for
definitions that should not be used by clients and are not covered by
API guarantees.

Note: As of now, all header files of libQuotient are considered public; this may change eventually.

### Code formatting

The code style is defined by `.clang-format`, and in general, all C++ files
should follow it. Files with minor deviations from the defined style are still
accepted in PRs; however, unless explicitly marked with `// clang-format off`
and `// clang-format on`, these deviations will be rectified any commit soon
after.

Additional considerations:
* 4-space indents, no tabs, no trailing spaces, no last empty lines. If you
  spot the code abusing these - thank you for fixing it.
* Prefer keeping lines within 80 characters. Slight overflows are ok only
  if that helps readability.
* Please don't make "hypocritical structs" with protected or private members.
  In general, `struct` is used to denote a plain-old-data structure, rather
  than data+behaviour. If you need access control or are adding yet another
  non-trivial (construction, assignment) member function to a `struct`,
  just make it a `class` instead.
* For newly created classes, keep to
  [the rule of 3/5/0](http://en.cppreference.com/w/cpp/language/rule_of_three) -
  make sure to read about the rule of zero if you haven't before, it's not
  what you might think it is.
* Qt containers are generally preferred to STL containers; however, there are
  notable exceptions, and libQuotient already uses them:
  * `std::array` and `std::deque` have no direct counterparts in Qt.
  * Because of COW semantics, Qt containers cannot hold uncopyable classes.
    Classes without a default constructor are a problem too. Examples of that
    are `SyncRoomData` and `EventsArray<>`. Use STL containers for those but
    see the next point and also consider if you can supply a reasonable
    copy/default constructor.
  * STL containers can be freely used in code internal to a translation unit
    (i.e., in a certain .cpp file) _as long as that is not exposed in the API_.
    It's ok to use, e.g., `std::vector` instead of `QVector` to tighten up
    code where you don't need COW, or when dealing with uncopyable
    data structures (see the previous point). However, exposing STL containers
    in the API is not encouraged (except where absolutely necessary, e.g. we use
    `std::deque` for a timeline). Exposing STL containers or iterators in API
    intended for usage by QML code (e.g. in `Q_PROPERTY`) is unlikely to work
    and therefore unlikely to be accepted into `master`.
* Although `std::unique_ptr<>` gives slightly stronger guarantees,
  `QScopedPointer<>` is better supported by Qt Creator's debugger UI and
  therefore is preferred.
* Use `QVector` instead of `QList` where possible - see the
  [great article by Marc Mutz on Qt containers](https://marcmutz.wordpress.com/effective-qt/containers/)
  for details.

### Automated tests

There's no testing framework as of now; either Catch or Qt Test or both will
be used eventually.

As a stopgap measure, qmc-example is used for automated functional testing.
Therefore, any significant addition to the library API should be accompanied
by a respective test in qmc-example. To add a test you should:
- Add a new private slot to the `QMCTest` class.
- Add to the beginning of the slot the line `running.push_back("Test name");`.
- Add test logic to the slot, using `QMC_CHECK` macro to assert the test outcome. ALL (even failing) branches should conclude with a QMC_CHECK invocation, unless you intend to have a "DID NOT FINISH" message in the logs under certain conditions.
- Call the slot from `QMCTest::startTests()`.

`QMCTest` sets up some basic test fixture to help you with testing; notably by the moment `startTests()` is invoked you can rely on having a working connection in `c` member variable and a test room in `targetRoom` member variable. PRs to introduce a proper testing framework are very welcome (make sure to migrate tests from qmc-example though); shifting qmc-example to use Qt Test seems to be a particularly low-hanging fruit.

### Security, privacy, and performance

Pay attention to security, and work *with* (not against) the usual security hardening mechanisms (however few in C++).

`char *` and similar unchecked C-style read/write arrays are forbidden - use Qt containers or at the very least `std::array<>` instead. Where you see fit (usually with data structures), try to use smart pointers, especially `std::unique_ptr<>` or `QScopedPointer` instead of bare pointers. When dealing with `QObject`s, use the parent-child ownership semantics exercised by Qt (this is preferred to using smart pointers). Shared pointers are not used in the code so far; but if you find a particular use case where the strict semantic of unique pointers doesn't help and a shared pointer is necessary, feel free to step up with the working code and it will be considered for inclusion.

Exercise the [principle of least privilege](https://en.wikipedia.org/wiki/Principle_of_least_privilege) where reasonable and appropriate. Prefer less-coupled cohesive code.

Protect private information, in particular passwords and email addresses. Absolutely _don't_ spill around this information in logs - use `access_token` and similar opaque ids instead, and only display those in UI where needed. Do not forget about local access to data (in particular, be very careful when storing something in temporary files, let alone permanent configuration or state). Avoid mechanisms that could be used for tracking where possible (we do need to verify people are logged in but that's pretty much it), and ensure that third parties can't use interactions for tracking. Matrix protocols evolve towards decoupling the personally identifiable information from user activity entirely - follow this trend.

We want the software to have decent performance for typical users. At the same time we keep libQuotient single-threaded as much as possible, to keep the code simple. That means being cautious about operation complexity (read about big-O notation if you need a kickstart on the topic). This especially refers to operations on the whole timeline and the list of users - each of these can have tens of thousands of elements so even operations with linear complexity, if heavy enough, can produce noticeable GUI freezing. When you don't see a way to reduce algorithmic complexity, embed occasional `processEvents()` invocations in heavy loops (see `Connection::saveState()` to get the idea).

Having said that, there's always a trade-off between various attributes; in particular, readability and maintainability of the code is more important than squeezing every bit out of that clumsy algorithm. Beware of premature optimization and have profiling data around before going into some hardcore optimization.

Speaking of profiling logs (see README.md on how to turn them on) - in order
to reduce small timespan logging spam, there's a default limit of at least
200 microseconds to log most operations with the PROFILER
(aka quotient.profile.debug) logging category. You can override this
limit by passing the new value (in microseconds) in PROFILER_LOG_USECS to
the compiler. In the future, this parameter will be made changeable at runtime
_if_ needed.

## How to check proposed changes before submitting them

Checking the code on at least one configuration is essential; if you only have
a hasty fix that doesn't even compile, better make an issue and put a link to
your commit into it (with an explanation what it is about and why).

### Standard checks

The following warnings configuration is applied with GCC and Clang when using CMake:
`-W -Wall -Wextra -pedantic -Werror=return-type -Wno-unused-parameter -Wno-gnu-zero-variadic-macro-arguments`
(the last one is to mute a warning triggered by Qt code for debug logging).
We don't turn most of the warnings to errors but please treat them as such.
In Qt Creator, the following line can be used with the Clang code model
(before Qt Creator 4.7 you should explicitly enable the Clang code model plugin):
`-Weverything -Werror=return-type -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-unused-macros -Wno-newline-eof -Wno-exit-time-destructors -Wno-global-constructors -Wno-gnu-zero-variadic-macro-arguments -Wno-documentation -Wno-missing-prototypes -Wno-shadow-field-in-constructor -Wno-padded -Wno-weak-vtables -Wno-unknown-attributes -Wno-comma`.

### Continuous Integration

We use Travis CI to check buildability and smoke-testing on Linux (GCC, Clang) and MacOS (Clang), and AppVeyor CI to build on Windows (MSVC). Every PR will go through these, and you'll see the traffic lights from them on the PR page. If your PR fails on any platform double-check that it's not your code causing it - and fix it if it is.

### clang-format

We strongly recommend using clang-format or, even better, use an IDE that
supports it. This will lay a tedious task of following the assumed
code style from your shoulders over to your computer.

### Other tools

Recent versions of Qt Creator and CLion can automatically run your code through
clang-tidy. The following list of clang-tidy checks slows Qt Creator analysis
quite considerably but gives a good insight without too many false positives:
`-*,bugprone-argument-comment,bugprone-assert-side-effect,bugprone-bool-pointer-implicit-conversion,bugprone-copy-constructor-init,bugprone-dangling-handle,bugprone-fold-init-type,bugprone-forward-declaration-namespace,bugprone-inaccurate-erase,bugprone-integer-division,bugprone-move-forwarding-reference,bugprone-string-constructor,bugprone-undefined-memory-manipulation,bugprone-use-after-move,bugprone-virtual-near-miss,cert-dcl03-c,cert-dcl21-cpp,cert-dcl50-cpp,cert-dcl54-cpp,cert-dcl58-cpp,cert-env33-c,cert-err09-cpp,cert-err34-c,cert-err52-cpp,cert-err60-cpp,cert-err61-cpp,cert-fio38-c,cert-flp30-c,cert-msc30-c,cert-msc50-cpp,cert-oop11-cpp,cppcoreguidelines-c-copy-assignment-signature,cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-slicing,hicpp-deprecated-headers,hicpp-invalid-access-moved,hicpp-member-init,hicpp-move-const-arg,hicpp-named-parameter,hicpp-new-delete-operators,hicpp-static-assert,hicpp-undelegated-constructor,hicpp-use-*,misc-misplaced-const,misc-new-delete-overloads,misc-non-copyable-objects,misc-redundant-expression,misc-static-assert,misc-throw-by-value-catch-by-reference,misc-unconventional-assign-operator,misc-uniqueptr-reset-release,misc-unused-*,modernize-loop-convert,modernize-pass-by-value,modernize-return-braced-init-list,modernize-shrink-to-fit,modernize-unary-static-assert,modernize-use-*,performance-faster-string-find,performance-for-range-copy,performance-implicit-conversion-in-loop,performance-inefficient-*,performance-move-*,performance-type-promotion-in-math-fn,performance-unnecessary-*,readability-delete-null-pointer,readability-else-after-return,readability-inconsistent-declaration-parameter-name,readability-misleading-indentation,readability-redundant-*,readability-simplify-boolean-expr,readability-static-definition-in-anonymous-namespace,readability-uniqueptr-delete-release`.

Qt Creator, in addition, knows about clazy, an even deeper Qt-aware static
analysis tool. Even level 1 clazy eats away CPU but produces some very relevant
notices that are easy to overlook otherwise, such as possible unintended copying
of a Qt container, or unguarded null pointers. You can use this time to time
(see Analyze menu in Qt Creator) instead of hogging your machine with
deep analysis as you type.

## Git commit messages

When writing git commit messages, try to follow the guidelines in
[How to Write a Git Commit Message](https://chris.beams.io/posts/git-commit/):

1.  Separate subject from body with a blank line
2.  Be reasonable on the subject line length, because this is what we see in commit logs. Try to fit in 50 characters whenever possible.
3.  Capitalize the subject line
4.  Do not end the subject line with a period
5.  Use the imperative mood in the subject line (*command* form)
  (we don't always practice this ourselves but let's try).
6.  Use the body to explain what and why vs. how
    (git tracks how it was changed in detail, don't repeat that). Sometimes a quick overview of "how" is acceptable if a commit is huge - but maybe split a commit into smaller ones, to begin with?

## Reuse (libraries, frameworks, etc.)

C++ is unfortunately not very coherent about SDK/package management, and we try to keep building the library as easy as possible. Because of that we are very conservative about adding dependencies to libQuotient. That relates to additional Qt components and even more to other libraries. Fortunately, even the Qt components now in use (Qt Core and Network) are very feature-rich and provide plenty of ready-made stuff.

Regardless of the above paragraph (and as mentioned earlier in the text), we're now looking at possible options for futures and automated testing, so PRs onboarding those will be considered with much gratitude.

Some cases need additional explanation:
* Before rolling out your own super-optimised container or algorithm written
  from scratch, take a good long look through documentation on Qt and
  C++ standard library. Please try to reuse the existing facilities
  as much as possible.
* You should have a good reason (or better several ones) to add a component
  from KDE Frameworks. We don't rule this out and there's no prejudice against
  KDE; it just so happened that KDE Frameworks is one of most obvious
  reuse candidates but so far none of these components survived
  as libQuotient deps. So we are cautious. Extra notice to KDE folks:
  I'll be happy if an addon library on top of libQuotient is made using
  KDE facilities, and I'm willing to take part in its evolution; but please
  also respect LXDE people who normally don't have KDE frameworks installed.
* Never forget that libQuotient is aimed to be a non-visual library;
  QtGui in dependencies is only driven by (entirely offscreen) dealing with
  QImages. While there's a bunch of visual code (in C++ and QML) shared
  between Quotient-enabled _applications_, this is likely to end up
  in a separate (Quotient-enabled) library, rather than libQuotient itself.

## Attribution

This text is based on CONTRIBUTING.md from CII Best Practices Badge project, which is a collective work of its contributors (many thanks!). The text itself is licensed under CC-BY-4.0.
