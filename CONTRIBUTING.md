# Contributing

Feedback and contributions are very welcome!

Here's help on how to make contributions, divided into the following sections:

* general information,
* vulnerability reporting,
* code changes,
* documentation,
* how to check proposed changes before submitting them,
* reuse (supply chain for third-party components, including updating them), and
* keeping up with external changes.

## General information

For specific proposals, please provide them as
[pull requests](https://github.com/QMatrixClient/libqmatrixclient/pulls)
or
[issues](https://github.com/QMatrixClient/libqmatrxclient/issues)
For general discussion, feel free to use our Matrix room:
[#quaternion:matrix.org](https://matrix.to/#/#quaternion:matrix.org).

If you're new to the project (or FLOSS in general),
[issues tagged as simple](https://github.com/QMatrixClient/libqmatrixclient/labels/simple)
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
[issue tracker](https://github.com/QMatrixClient/libqmatrixclient/issues) and
[pull requests](https://github.com/QMatrixClient/libqmatrixclient/pulls).
Specific changes are proposed using those mechanisms.
Issues are assigned to an individual, who works it and then marks it complete.
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

Unless a contributor explicitly specifies otherwise, we assume that all contributed code is released under [the same license as libqmatrixclient itself](./COPYING), which is LGPL v2.1 as of the time of this writing.
<!-- The below is invalid yet!
All new contributed material that is not executable, including all text when not executed, is also released under the
[Creative Commons Attribution 4.0 International (CC BY 4.0) license](https://creativecommons.org/licenses/by/3.0/) or later.
-->

Any components proposed for reuse should have a license that permits releasing a derivative work under LGPL v2.1. Moreover, the license of a proposed component should be approved by OSI, no exceptions.

## Vulnerability reporting (security issues)

If you find a significant vulnerability, or evidence of one,
use either of the following contacts:
* send an email to Kitsune Ral [Kitsune-Ral@users.sf.net](mailto:Kitsune-Ral@users.sf.net)
* reach out in Matrix to #kitsune:matrix.org (if you can, switch encryption **on**)

In any of these two options, _indicate that you have such
information_ (do not share the information yet), and we'll tell you the next steps.

We will gladly give credit to anyone who reports a vulnerability so that we can fix it. If you want to remain anonymous or pseudonymous instead, please let us know that; we will gladly respect your wishes.

## Code changes

The code should strive to be DRY (don't repeat yourself), clear, and obviously correct. Some technical debt is inevitable, just don't bankrupt us with it. Refactoring is welcome.

### Qt-flavoured C++

This is our primary language. We don't have a particular code style _as of yet_ but some rules-of-thumb are below:
* 4-space indents, no tabs, no trailing spaces, no last empty lines. If you spot the code abusing these - we'll thank you for fixing it.
* Lines within 80 characters are preferred.
* Braces after if's, while's, do's, function signatures etc. take a separate line.
* A historical deviation from the usual Qt code format conventions is an extra indent inside _classes_ (access specifiers go at +4 spaces to the base, members at +8 spaces) but not _structs_ (members at +4 spaces). This may change in the future for something more conventional.
* Please don't make hypocritical structs with protected or private members. Just make them classes instead.
* Qt containers are generally preferred to STL containers; however, there are notable exceptions, and libqmatrixclient already uses them:
  * `std::array` and `std::deque` have no direct counterparts in Qt.
  * Because of COW semantics, Qt containers cannot hold uncopyable classes. Classes without a default constructor are a problem too. An example of that is `SyncRoomData` and `EventsBatch<>`. Use standard containers for those but see the next point and also consider if you can supply a reasonable copy/default constructor.
  * Standard containers can be used in code internal to a translation unit (i.e., in a certain .cpp file) _as long as it's not exposed in the interface_. It's ok to use, e.g., `std::vector` instead of `QVector` in tighter code, or when dealing with uncopyable (see the previous point) elements. However, exposing standard containers in the API that might be used by QML will not work at all and will never be accepted.
* Use `QVector` instead of `QList` where possible - see a [great article of Marc Mutz on Qt containers](https://marcmutz.wordpress.com/effective-qt/containers/) for details. 

### Automated tests

There's no testing framework as of now; either Catch or QTest or both will be used eventually (PRs welcome, just don't expect a quick merge of one - we'll hunt you down to actually write some tests first :-D ).

### Security, privacy, and performance

Pay attention to security, and work *with* (not against) the usual security hardening mechanisms (however few in C++). `char *` and similar unchecked C-style read/write arrays are forbidden - use Qt containers or at the very least `std::array<>` instead.

Exercise the [principle of least privilege](https://en.wikipedia.org/wiki/Principle_of_least_privilege) where reasonable and appropriate. Prefer less-coupled cohesive code.

Protect private information, in particular passwords and email addresses. Do not forget about local access to data (in particular, be very careful when storing something in temporary files, let alone permanent configuration or state). Avoid mechanisms that could be used for tracking where possible (we do need to verify people are logged in but that's pretty much it), and ensure that third parties can't use interactions for tracking.

We want the software to have decent performance for typical users. At the same time we keep libqmatrixclient single-threaded as much as possible, to keep the code simple. That means being cautious about operation complexity (read about big-O notation if you need a kickstart on the topic). This especially refers to operations on the whole timeline - it can easily be several thousands elements long so even operations with linear complexity, if heavy enough, can produce noticeable GUI freezing.

Having said that, there's always a trade-off between various attributes; in particular, readability and maintainability of the code is more important than squeezing every bit out of that clumsy algorithm.

## Documentation changes

Most of the documentation is in "markdown" format.
All markdown files use the .md filename extension.

Where reasonable, limit yourself to Markdown
that will be accepted by different markdown processors
(e.g., what is specified by CommonMark or the original Markdown).
In practice we use the version of Markdown implemented by GitHub when it renders .md files, and you can use its extensions (in particular, mark code snippets with the programming language used).
This version of markdown is sometimes called
[GitHub-flavored markdown](https://help.github.com/articles/github-flavored-markdown/).
In particular, blank lines separate paragraphs; newlines inside a paragraph do *not* force a line break.
Beware - this is *not* the same markdown algorithm used by GitHub when it renders issue or pull comments; in those cases [newlines in paragraph-like content are considered as real line breaks](https://help.github.com/articles/writing-on-github/); unfortunately this other algorithm is *also* called GitHub rendered markdown. (Yes, it'd be better if there were standard different names for different things.)

In your markdown, please don't use tab characters and avoid "bare" URLs (in a hypertext link, the link text and URL should be on the same line). We do not care about the line length in markdown texts (and more often than not put the whole paragraph into one line). This is actually negotiable, and absolutely not enforceable. If you want to fit in a 70-character limit, go ahead, just don't reformat the whole text along the way. Take care to not break URLs when breaking lines.

Do not use trailing two spaces for line breaks, since these cannot be seen and may be silently removed by some tools. Instead, use <tt>&lt;br&nbsp;/&gt;</tt> (an HTML break).

## How to check proposed changes before submitting them

Checking the code on at least one configuration is essential; if you only have a hasty fix that doesn't even compile, better make an issue and put a link to your commit into it (with an explanation what it is about and why).

### Standard checks

`-Wall -pedantic` is used with GCC and Clang. We don't turn those warnings to errors but please treat them as such.

### Continuous Integration

We use Travis CI to check buildability on Linux (GCC, Clang) and MacOS (Clang), and AppVeyor CI to build on Windows (MSVC). Every PR will go through these, and you'll see the traffic lights from them on the PR page. Failure on any platform will most likely entail a request to you for a fix before merging a PR.

### Other tools

If you know how to use clang-tidy, here's a list of checks we do and do not use (a leading hyphen means a disabled check, an asterisk is a wildcard): `*,cert-env33-c,-cppcoreguidelines-pro-bounds-array-to-pointer-decay,-cppcoreguidelines-pro-bounds-constant-array-index,-cppcoreguidelines-pro-bounds-pointer-arithmetic,-cppcoreguidelines-pro-type-const-cast,-cppcoreguidelines-pro-type-union-access,-cppcoreguidelines-special-member-functions,-google-build-using-namespace,-google-readability-braces-around-statements,-hicpp-*,-llvm-*,-misc-unused-parameters,-misc-noexcept-moveconstructor,-modernize-use-using,-readability-braces-around-statements,readability-identifier-naming,-readability-implicit-bool-cast,-clang-diagnostic-*,-clang-analyzer-*`. If you're on CLion (which makes clang-tidy usage a no-brainer), you can simple copy-paste the above list into the Clang-Tidy inspection configuration.

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

C++ is unfortunately not very coherent about SDK/package management, and we try to keep building the library as easy as possible. Because of that we are very (and it means _very_) conservative about adding dependencies to libqmatrixclient. That relates to both additional Qt components and even more to other libraries.

Regardless of the above paragraph (and as mentioned earlier in the text), we're now looking at possible options for automated testing, so PRs onboarding a test framework will be considered with much gratitude. Just don't forget to bootstrap with at least some tests on the existing codebase, rather than just throw in a Git submodule.

Some cases need additional explanation:
* Before rolling out your own super-optimised container or algorithm written from scratch, take a good long look through documentation on Qt and C++ standard library (including the experimental/future sections). Please try to reuse the existing facilities as much as possible.
* You should have a good reason (or better several ones) to add a component from KDE Frameworks. We don't rule this out and there's no prejudice against KDE; it just so happened that KDE Frameworks is one of most obvious reuse candidates for us but so far none of these components survived as libqmatrixclient deps.
* Never forget that libqmatrixclient is aimed to be a non-visual library; QtGui in dependencies is only driven by (entirely offscreen) dealing with QPixmaps. While there's a bunch of visual code (in C++ and QML) shared between libqmatrixclient-enabled _applications_, this is likely to end up in a separate (libqmatrixclient-enabled) library, rather than libqmatrixclient.

## Attribution

This text is largely based on CONTRIBUTING.md from CII Best Practices Badge project, which is a collective work of its contributors (many thanks!). The text itself is licensed under CC-BY-4.0.
