# Contributing

Feedback and contributions are very welcome!

Here's help on how to make contributions, divided into the following sections:

The quick-read part:
* general information,
* vulnerability reporting,
* code conventions overview,
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

If you're new to the project (or FLOSS in general), here's 
[the list of issues](https://github.com/quotient-im/libQuotient/labels/good%20first%20issue)
that we consider simple enough for starters - they don't require much knowledge
about the project and should be relatively straightforward (but not necessarily
quick to fix). Welcome aboard!

### Pull requests and different branches recommended

Contributions are preferred in the form of pull requests at GitHub.
See the respective
[articles about pull requests](https://help.github.com/articles/using-pull-requests/)
to learn how to deal with them.

We recommend creating different branches for different (logical)
changes, and creating a pull request when you're done; the development
integration branch is `dev`. See the GitHub documentation on
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
a derivative work under *LGPL v3 or later* (that includes licenses permitting
*LGPL v2.1 or later* but not *LGPL v2.1 only*). In any case, the component
should be redistributable under a license from
[the list approved by OSI](https://opensource.org/licenses), no exceptions.

We use [SPDX](https://spdx.dev) conventions for copyright statements. Please
follow them when making a sizable contribution: add your name and year to
the top of the file. New files should begin with the following preamble:
```cpp
// SPDX-FileCopyrightText: 2021 Your Name <your@email.address>
// SPDX-License-Identifier: LGPL-2.1-or-later
```

## Vulnerability reporting (security issues) - see [SECURITY.md](./SECURITY.md)


## Code changes

The code should strive to be DRY (don't repeat yourself), clear, and syntactically
correct (i.e. buildable). Some technical debt is inevitable but glaring
inconsistencies, duplications etc. will most likely cause a request for changes
at the pull request review. Refactoring is always welcome; if you do it within
another feature development, please arrange the refactoring before the feature,
to make the review easier.

### C++ feature set

As of Quotient 0.8, the C++ standard for newly written code is C++20. Since none
of the supported compilers (GCC 11, Clang 11, Apple Clang 12, MSVC 19.30 - see
also the pre-requisites in [README](./README.md)) can handle the entire C++20
feature set, we have to stick with a subset. Most notably:

- `std::bind_front`, ranges, `std::format`, `std::source_location` can't be used
  yet;
- while concepts and constraints as a language feature are there, most library
  concepts are not available as of Apple Clang 12;
- No `constexpr` containers except `std::array` (but you can, and should,
  use `QLatin1String` and `Quotient::operator""_ls` that creates it, for
  constexpr Latin-1 strings).

The [compiler support page](https://en.cppreference.com/w/cpp/compiler_support#cpp20),
of cppreference.com, combined with the list of compiler versions above, can be
used to check whether a given feature is there. Be mindful that Clang build
configuration on Linux does not use Clang libc++ but rather the GNU standard
library (i.e. you should look at Clang column for core language features but
GCC libstdc++ for library features).

### Code style and formatting

The code style is defined by `.clang-format`, and in general, all C++ files
should follow it. Reasonable deviations from the defined style are allowed;
use `// clang-format off` and `// clang-format on` to protect them.

Most fundamental things from `.clang-format`:
* We (mostly) use Webkit style: 4-space indents, no tabs, no trailing spaces, no last empty lines.
  If you spot code that doesn't follow this, fix it on the spot, thank you.
* Prefer keeping lines within 100 characters. Slight overflows are ok if that
  helps readability. Ideally, just use `clang-format` to format lines.

### API conventions

All non-inline symbols (functions, classes/structs, even namespace-level static
variables) that are intended for use in client code must be declared with
`QUOTIENT_API` macro (the macro itself is defined in the dedicated
`quotient_export.h` file). This is concerned with symbols visibility in
dynamic/shared libraries: the macro marks these symbols for exporting in
the library symbol table. If you forget to use this macro where needed you will 
get linkage errors if you're lucky, obscure runtime errors otherwise (such as
split-brained singleton instances). You only need to use the macro on the 
namespace level; inner symbols (member functions, e.g.) are exported if 
their class is exported.

Some header files of the library are not intended to be (directly) included by
clients - these header files have names ending with `_p.h` (e.g. 
`connection_p.h`). All other header files are considered a part of the library
official API (and, respectively, ABI). Calls, data structures and other 
symbols _not_ intended for usage in client code should _not_ be exposed in
public header files, unless there's no way to keep them out. In particular, 
this includes private members (functions, typedefs, or variables) in public 
classes; use pimpl idiom to hide implementation details as much as possible.
For cases when a definition has to be in the header file but should not be used
in client code, use `_impl` namespace - anything in that namespace is not 
covered by API guarantees either (but may still contribute to the ABI surface,
so tread carefully).

### Generated C++ code for CS API

The code in `Quotient/csapi`, `Quotient/identity` and
`Quotient/application-service`, although stored in Git, is actually generated
from the official Matrix Client-Server API definition files. Make sure to read
[CODE_GENERATION.md](./CODE_GENERATION.md) before trying to change anything
there.


## Documentation changes

Most of the documentation is in Markdown format. All Markdown files use the 
`.md` filename extension.

Where reasonable, limit yourself to Markdown that will be accepted by different
markdown processors (e.g., what is specified by CommonMark or the original
Markdown). In practice, as long as libQuotient is hosted at GitHub,
[GFM (GitHub-flavoured Markdown)](https://help.github.com/articles/github-flavored-markdown/)
is used to show those files in a browser, so it's fine to use its extensions.
In particular, you can mark code snippets with the programming language used;
blank lines separate paragraphs, newlines inside a paragraph do *not* force
a line break.

Be mindful that this is *not* the same (albeit similar) markdown algorithm used
by GitHub when it renders issue or pull comments; in those cases
[newlines in paragraph-like content are considered as real line breaks](https://help.github.com/articles/writing-on-github/);
unfortunately this other algorithm is *also* called GitHub-flavoured markdown.
(Yes, it would be better if there were different names for different things.)

In your markdown, please don't use tab characters and avoid "bare" URLs.
In a hyperlink, the link text and URL should be on the same line.
Both in C/C++ code comments and Markdown documents, try to keep your lines
within the 100-character limit _except hyperlinks_ (wrapping breaks them). Some
historical text may not follow that rule - feel free to reformat those parts
when you edit them.

Do not use trailing two spaces for line breaks, since these cannot be seen
and may be silently removed by some tools. If, for whatever reason, a blank line
is not an option, use <tt>&lt;br/&gt;</tt> (an HTML line break).


## End of TL;DR

If you don't plan/have substantial contributions, you can stop reading here.
Further sections are for those who's going to actively hack on the library code.


## Code changes

### More on code style and formatting

* Do not use `struct` when you have protected or private members; only use it
  to define plain-old-data structures, with maybe just a function or two among 
  public members but no substantial behaviour. If you need access control or 
  specific logic tightly coupled to the data structure, make it a `class`
  instead and consider if you still want to keep its member variables public.

* For newly created classes, keep to
  [the rule of 3/5/0](http://en.cppreference.com/w/cpp/language/rule_of_three).

* Qt containers are generally preferred to STL containers, with exceptions 
  listed below:
  * `std::array` and `std::deque` have no direct counterparts in Qt and are 
    unconditionally accepted in the backend code. They can't be mapped to 
    QML though; see another bullet below for implications of that.
  * Because of COW semantics, Qt containers cannot hold uncopyable classes.
    Classes without a default constructor are a problem too. Examples of that
    are `SyncRoomData` and `EventsArray<>`. Again, you can use STL containers 
    for structures having those but consider the implications.
  * So, the implications. Because QML doesn't know about most of STL containers and cannot pull data
    out of them, you're only free to use STL containers in backend code (in the simplest case,
    within one .cpp file). The API exposing these containers can only be used from C++ code, with
    `std::vector` being a notable exception that QML knows about (but you still can't read
    uncopyable vectors such as `EventsArray<>`, as the previous bullet already said). For these
    cases you have to provide external means to iterate through the container and consume data
    from it; exposing a Qt item model is most natural to Qt code. If you don't provide such other
    means, expect questions at your pull request.
  * Notwithstanding the above (you're not going to use smart pointers with QML 
    anyway), prefer `std::unique_ptr<>` over `QScopedPointer<>` as it gives
    stronger guarantees; also, some features of `QScopedPointer` are deprecated
    in Qt 6.

* Always use `QVector` instead of `QList` unless Qt's own API uses it - see the
  [great article by Marc Mutz on Qt containers](https://marcmutz.wordpress.com/effective-qt/containers/)
  for details. With Qt 6, these two become the same type matching what used
  to be `QVector` in Qt 5.

  (Note: unfortunately, `QVector` is a type alias in Qt 6 and that breaks
  templated code because type deduction doesn't work with aliases. This breakage
  will go away as compilers adopt C++23 sufficiently but it may take a 
  couple more years, as of this writing; in the meantime, the fix boils down
  to specifying the template parameter of `QVector` explicitly.)

* When you write logs within the library always use logging categories defined in
  `logging_categories_p.h` instead of plain `qDebug()`, to avoid a log line being assigned
  the default category. `qCDebug(CATEGORY)` is the preferred form; `qDebug(CATEGORY)` (without `C`)
  is accepted as well. Do not add new logging categories without necessity; if you do, make sure
  to add the new category to `logging_categories_p.h`, so that there's a central reference for all
  of them (mentioned in README.md, by the way).

### Comments

Whenever you add a new call to the library API that you expect to be used
from client code, make sure to supply a proper doc-comment along with the call.
Quotient uses the Doxygen C++-styled doc-comments (`//!`, `\brief`); some legacy
code may use Javadoc (`/** ... */`, `@brief`) or C-styled Doxygen (`/*! ... */`)
but that is not encouraged any more. Some parts are not documented at all;
adding doc-comments to them and/or converting the existing ones to the assumed
style is highly encouraged; it's also a nice and easy first-time contribution.

Use `\brief` for the summary, and follow with details after
an empty doc-comment line, using `\param`, `\return` etc. as necessary.

When commenting in-code:
* Don't restate what's happening in the code unless it's not really obvious.
  We assume the readers to have some command of C++ and Qt. If your code is
  not obvious, consider making it clearer itself before commenting.
* That said, both C++ and Qt have their arcane/novel features and dark corners, and education of
  code readers is a great thing. Use your experience to figure what might be not that well-known,
  and comment such cases: leave references to web pages, Quotient wiki etc. Do not comment `std::`
  calls just because they are less known - readers are expected to know about cppreference.com and
  look it up.
* More important than everything above - make sure to document not so much "what" but more "why"
  certain code is done the way it is. In the worst case, the logic of the code can be
  reverse-engineered; but you can almost never reverse-engineer the line of reasoning and
  the pitfalls avoided.

### Automated tests

We gradually introduce autotests based on a combination of CTest and Qt Test
frameworks - see `autotests/` directory. There are very few of those, as we
have just started adding those to the new code (you guessed it; adding more
tests to the old code is very welcome and also is a good exercise to get to
know the library).

On top of that, libQuotient comes with a command-line end-to-end test suite
called Quotest. Any significant addition to the library API should be
accompanied by a respective test in `autotests/` and/or in Quotest.

To add a test to autotests:
- In a new `.cpp` file in `autotests/` (you don't need a header file), define a test class derived
  from `QObject` and write tests as member functions in its `private slots:` section. See other
  autotests to get an idea of what it should look like.
- Add a `quotient_add_test` macro call with your test to `autotests/CMakeLists.txt`

To add a test to Quotest:
- In `quotest.cpp`, add a new test to the `TestSuite` class. Similar to Qt Test,
  each test in Quotest is a private slot; unlike Qt Test, you should use
  special macros, `TEST_DECL()` and `TEST_IMPL()`, to declare and define
  the test (those macros conceal passing the testing handle in `thisTest`
  variable to the test method).
- In the test function definition, add test logic using `FINISH_TEST` macro
  to check for the test outcome and complete the test (be mindful that
  `FINISH_TEST` always `return`s, not only in case of error). ALL (even failing)
  branches should conclude with a `FINISH_TEST` (or `FAIL_TEST` that is
  a shortcut for a failing `FINISH_TEST`) invocation, unless you want the test
  to fail with a "DID NOT FINISH" message in the logs under certain conditions.

The `TestManager` class sets up some basic test fixture to help you with testing;
notably, the tests can rely on having an initialised `Room` object with loaded
state for the test room in `targetRoom` member variable. Note that it's normal
for tests to go async, which is not something Qt Test is easy with (and this
is why Quotest doesn't directly use Qt Test but rather fetches a few ideas
from it).

### Security and privacy

Pay attention to security, and work *with*, not against, the usual security
hardening practices.

`char *` and similar unchecked C-style read/write arrays are forbidden - use Qt containers
(`QString`/`QLatin1String` for strings, in particular) or `std::array<>`/`std::span<>` instead
(E2EE-related code can also use handy `byte_view_t` and `byte_span_t` aliases). When dealing with
`QObject`s, organise them in parent-child trees and let Qt manage object lifecycles for you instead
explicit deletions. If that doesn't work in a given situation (no obvious parent, non-trivial
lifecycle, the object doesn't derive from `QObject`, etc.), try to at least use `std::unique_ptr<>`
and `std::move()` it when/where appropriate to make sure the object has clear ownership. Finally,
shared pointers can be used when circular dependencies are not a concern. Avoid direct pointer
arithmetic wherever possible and only use bare pointers within `QObject` parent-child trees and
for non-owning/weak access to resources owned by `std::unique_ptr<>`. Consider using references
instead of pointers, where applicable.

Exercise the
[principle of least privilege](https://en.wikipedia.org/wiki/Principle_of_least_privilege)
where reasonable and appropriate. Prefer internally cohesive code while avoiding
too much external coupling.

Avoid dealing with any personal information (e.g. email addresses) and secrets 
(passwords, tokens, keys) unless it is the specific and exclusive purpose
of the given code. Absolutely _don't_ spill around this information in logs, 
and only display those in UI where really needed. Do not forget about the 
issue of local access (in particular, be very careful when storing something in 
temporary files, let alone permanent configuration or state).

Where possible, avoid mechanisms that could be used for user tracking (we do need
to verify people are logged in but that's pretty much it), and ensure that 
third parties can't use interactions for tracking. Matrix protocols evolve 
towards decoupling personal information from user activity entirely - follow
this trend.

### Performance

We want the software to have decent performance for users even on weaker
machines. At the same time we keep libQuotient single-threaded as much
as possible, to keep the code simple. That means being cautious about operation
complexity (read about big-O notation if you need a kickstart on the subject).
This especially refers to operations on the whole timeline and the list of
users - each of these can have tens of thousands of elements so even operations
with linear complexity, if heavy enough (with I/O or complex processing),
can produce noticeable GUI freezing or stuttering. When you don't see a way
to reduce algorithmic complexity, either split processing into isolated
pieces that can be individually scheduled as queued events (see the end of
`Connection::consumeRoomData()` to get the idea) or uncouple the logic from
GUI and execute it outside of the main thread with `QtConcurrent` facilities.

Having said that, there's always a trade-off between various attributes;
in particular, readability and maintainability of the code is more important
than squeezing every bit out of a clumsy algorithm. Beware of premature
optimization and profile the code before diving into hardcore tweaking that 
might not give the benefits you think it would.

Speaking of profiling logs (see [README.md](./README.md) on how to turn them
on) - if you expect some code to take considerable (more than 10k "simple 
operations") time you might want to setup a `QElapsedTimer` and drop the 
elapsed time into logs under `PROFILER` logging category. See the existing 
code for examples - `room.cpp` has quite a few. In order to reduce small 
timespan logging spam, `PROFILER` log lines are usually guarded by a check 
that the timer counted big enough time (200 microseconds by default, 20 
microseconds for tighter parts); this threshold can be altered at compile-time 
by defining `PROFILER_LOG_USECS` preprocessor symbol (i.e. passing 
`-DPROFILE_LOG_USECS=<usecs>` to the compiler if you're on Linux/macOS).

## How to check proposed changes before submitting them

Checking the code on at least one configuration is essential; if you only have
a hasty fix that doesn't even compile, better make an issue and put a link to
your commit or gist into it (with an explanation what it is about and why).

### Compiler warnings

The warnings configuration applied when using CMake can be found in
`CMakeLists.txt`. Most warnings triggered by that configuration are not formally
considered errors (the compiler will keep going) but please treat them as such.

If you want the IDE to be _really_ picky about your code you can use
the following line for the Clang analyzer code model to enable most compiler
warnings while keeping the number of false positives at bay (that does not
include `clang-tidy`/`clazy` warnings - see the next section on those):
`-Weverything -Werror=return-type -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-c++20-compat -Wno-unused-macros -Wno-newline-eof -Wno-exit-time-destructors -Wno-global-constructors -Wno-gnu-zero-variadic-macro-arguments -Wno-documentation -Wno-missing-prototypes -Wno-shadow-field-in-constructor -Wno-padded -Wno-weak-vtables -Wno-unknown-attributes -Wno-comma -Wno-shadow-uncaptured-local -Wno-switch-enum -Wno-pragma-once-outside-header -Wno-range-loop-bind-reference -Wno-unsafe-buffer-usage`

### Static analysis tools

Many IDEs these days can automatically run your code through clang-tidy. The source code contains
`.clang-tidy` file with the recommended set of checks that doesn't give too many false positives.

Qt Creator in addition knows about clazy, a Qt-aware static analysis tool that
hunts for Qt-specific issues that are easy to overlook otherwise, such as
possible unintended copying of a Qt container. Most of clazy checks are relevant
to our code; here's the configuration line the author of this text is using:
`level2,no-foreach,no-non-pod-global-static,no-range-loop-reference,no-ctor-missing-parent-argument,no-missing-qobject-macro,no-jni-signatures,no-qt-keywords,no-qt4-qstring-from-array`

### Continuous Integration

We use GitHub Actions to check buildability and smoke-testing on Linux
(GCC, Clang), MacOS (Clang), and Windows (MSVC). Every PR will go through these,
and you'll see the traffic lights from them on the PR page. If your PR fails
on any platform double-check that it's not your code causing it - and fix
(or ask how to fix if you don't know) if it is. Generally, pull requests are 
not accepted until they build on all platforms.


## Git commit messages

When writing git commit messages, try to follow the guidelines in
[How to Write a Git Commit Message](https://chris.beams.io/posts/git-commit/):

1.  Separate subject from body with a blank line.
2.  Be reasonable on the subject line length, because this is what we see in
    commit logs that are constrained in horizontal space. Try to fit in
    50 characters whenever possible.
3.  Capitalize the subject line.
4.  Do not end the subject line with a period.
5.  Aim to use the imperative mood in the subject line (*command* form).
6.  Use the body to explain what and why vs. how (git tracks how it was changed
    in detail, don't repeat that). Sometimes a quick overview of "how" is 
    acceptable if a commit is huge - but maybe split a commit into smaller ones,
    to begin with?


## Reuse (libraries, frameworks, etc.)

SDK/package management is unfortunately messy in C++, and we try to keep building the library
as easy as possible. Besides, every additional dependency means additional attack surface and
more effort to stay on most recent versions of all deps - not just upstream but within each
ecosystem (Linux distros, Homebrew, etc.). Because of these considerations we are very conservative
about adding dependencies to libQuotient. That mainly relates to libraries external to Qt; you can
use most of non-visual Qt components as needed (with minor caveats mentioned below). Fortunately,
even the Qt components now in use (Qt Core and Qt Network) are very feature-rich and provide plenty
of ready-made stuff.

Some cases need additional explanation:
* Don't reinvent the wheel - look through documentation on Qt and C++ standard library and use
  existing facilities as much as possible. C++ standard in particular has grown considerably in
  recent years, providing many useful algorithms and primitives out of the box.
* libQuotient is a library to build Qt applications; for that reason, components from KDE Frameworks
  should be really lightweight and useful to be accepted as a dependency. If the intention is
  to better integrate libQuotient into KDE environment there's nothing wrong in building another
  library on top of libQuotient. Consider people who run LXDE or even GNOME and normally don't have
  KDE frameworks installed (some actively avoid installing those) - libQuotient caters to them too.
* Never forget that libQuotient is an offscreen library; it only depends on
  QtGui to handle `QImage` objects (entirely offscreen still). While there's
  a bunch of visual code (in C++ and QML) shared between Quotient-enabled
  _applications_, this is likely to end up in a separate (Quotient-backed)
  library, rather than in libQuotient itself.
* Also be mindful that libQuotient strives to be cross-platform: at least across the three most
  popular platforms (Linux, macOS, Windows) are supported officially, but we also try to stay
  friendly to other platforms, such as Android, BSD, or Haiku. If the introduced dependency (even
  from inside Qt - e.g. DBus on Linux) is unavailable on one of the platforms already supported,
  it can only be added to enable a platform-specific feature and should not break or disable
  functionality on other platforms.

## Attribution

This text was originally based on CONTRIBUTING.md from CII Best Practices Badge project,
which is a collective work of its contributors (many thanks!). The text is licensed under CC-BY-4.0.
