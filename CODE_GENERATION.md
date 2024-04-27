# Generated C++ code for CS API

The code in `Quotient/csapi`, `Quotient/identity` and
`Quotient/application-service`, although stored in Git, is actually generated
from the official Matrix Client-Server API definition files. If you're unhappy
with something in there and want to improve that, you have to understand the way
these files are produced and setup some additional tooling. The shortest
possible procedure resembling the below text can be found in
.github/workflows/ci.yml (regeneration of those files is tested in CI) - see
the tasks "Get CS API definitions; clone and build GTAD" and
"Regenerate API code".

## Why generate the code at all?

Because otherwise we have to do monkey business of writing boilerplate code,
with the same patterns, types etc., literally, for every single API endpoint,
and one of libQuotient authors got fed up with it at some point in time.
By then about 15 job classes have been written; the whole API is about 100
endpoints, with new ones added and some existing changed in each Matrix protocol
version. Other considerations can be found in
[this talk about API description languages](https://youtu.be/W5TmRozH-rg)
that also briefly touches on GTAD - the tool written for the purpose.

## Prerequisites for CS API code generation

1. Get the source code of GTAD and its dependencies. Since version 0.7,
   libQuotient includes GTAD as a submodule so you can get everything you need
   by updating gtad/gtad submodule in libQuotient sources:
   `git submodule update --init --recursive gtad/gtad`.

   You can also just clone GTAD sources to keep them separate from libQuotient:
   `git clone --recursive https://github.com/quotient-im/gtad.git`
2. Configure and build GTAD: same as libQuotient, it uses CMake so this should
   be very straightforward (if not - you're probably not quite ready for this
   stuff anyway).
3. Get Matrix CS API definitions from a matrix-spec repo. Although the official
   repo is at https://github.com/matrix-org/matrix-spec.git` (formerly
   https://github.com/matrix-org/matrix-doc.git), you cannot use its main branch
   with GTAD 0.9 (used for the current - 0.8 - version of libQuotient) because
   the project has recently moved to OpenAPI 3 and GTAD does not support it yet.
   For that reason as well as build predictability (the official Matrix repo
   doesn't check whether a particular change breaks code generation), a soft
   fork of the official definitions is kept at
   https://github.com/quotient-im/matrix-spec.git - that guarantees buildability
   of the generated code. This repo closely follows the official one (but maybe
   not its freshest commit), applying a few adjustments on top. And of course
   you can use your own repository if you need to change the API definition.
4. If you plan to submit a PR with the generated code to libQuotient or just
   would like it to be properly formatted, you should either ensure you have
   clang-format (version 13 at least) in your PATH or pass
   `-DCLANG_FORMAT=<path>` to CMake, as mentioned in the next section.

## Generating CS API contents

1. Pass additional configuration to CMake when configuring libQuotient:
   `-DMATRIX_SPEC_PATH=/path/to/matrix-spec/ -DGTAD_PATH=/path/to/gtad`.
   Note that `MATRIX_SPEC_PATH` should lead to the _repo_ (not the API folder)
   while `GTAD_PATH` should have the path to GTAD _binary_. If you need
   to specify where your clang-format is (see the previous section) add
   `-DCLANG_FORMAT=/path/to/clang-format` to the line above. If everything's
   right, the detected locations will be mentioned in CMake output and
   an additional build target called `update-api` will be configured.
2. Generate the code: `cmake --build <your build dir> --target update-api`.
   Building this target will create (overwriting without warning) source files
   in `Quotient/csapi`, `Quotient/identity`, `Quotient/application-service` for
   all YAML files it can find in `/path/to/matrix-spec/data/api/client-server`
   and their dependencies.

## Changing generated code

See the more detailed description of what GTAD is and how it works in
the documentation on GTAD in its source repo. Only parts specific for
libQuotient are described here.

GTAD uses the following three kinds of sources:
1. OpenAPI files. Each file is treated as a separate source (unlike
   swagger-codegen, you do _not_ need to have a single file for the whole API).
2. A configuration file, in Quotient case it's `gtad/gtad.yaml` - common for
   all OpenAPI files GTAD is invoked on.
3. Source code template files: `gtad/*.mustache` - are also common.

The Mustache files have a templated (not in C++ sense) definition of a network
job class derived from BaseJob; if necessary, data structure definitions used
by this job are put before the job class. Bigger Mustache files look a bit
daunting for a newcomer; and the only known highlighter that can handle
the combination of Mustache (originally a web templating language) and C++ can
be found in CLion IDE. Fortunately, all our Mustache files are reasonably
concise and well-formatted these days.
To simplify things some reusable Mustache blocks are defined in `gtad.yaml` -
see its `mustache:` section. Adventurous souls that would like to figure out
what's going on in these files should speak up in the Quotient room -
I (Kitsune) will be very glad to navigate you.

The `types` map in `gtad.yaml` defines a mapping from OpenAPI types to C++/Qt.
It uses the following type attributes aside from pretty obvious `imports:`:
* `avoidCopy` - this attribute defines whether a const ref should be used
  instead of a value. For basic types like int this is obviously unnecessary;
  but compound types like `QVector` should rather be taken by reference when
  possible.
* `moveOnly` - some types are not copyable at all and must be moved instead
  (an obvious example is any structure that uses, directly or indirectly,
  `std::unique_ptr<>`).
* `useOptional` - wrap types that have no value with "null" semantics (i.e. number types and
  custom-defined data structures) into `std::optional`.
* `omittedValue` - an alternative for `useOptional`, just provide a value used
  for an omitted parameter. This is used for bool parameters which normally are
  considered false if omitted (or they have an explicit default value, passed
  in the "official" GTAD's `defaultValue` variable).
* `initializer` - this is a _partial_ (see GTAD and Mustache documentation for
  explanations but basically it's a variable that is a Mustache template itself)
  that specifies how exactly a default value should be passed to the parameter.
  E.g., the default value for a `QString` parameter is enclosed into
  `QStringLiteral`.

Instead of relying on the event structure definition in the OpenAPI files,
`gtad.yaml` uses pointers to libQuotient's event structures: `EventPtr`,
`RoomEventPtr` and `StateEventPtr`. Respectively, arrays of events, when
encountered in OpenAPI definitions, are converted to `Events`, `RoomEvents`
and `StateEvents` containers. When there's no way to figure the type from
the definition, an opaque `QJsonObject` is used, leaving the conversion
to the library and/or client code.

## Submitting API changes

Getting the API changes upstream requires coordination across a few Matrix
projects (the API is a contract between the client and the server, after all).
The recommended sequence is different, mainly depending on on whether or not
you have to write a
[Matrix Spec Change aka MSC](https://matrix.org/docs/spec/proposals). Simply
speaking, if your changes don't break compatibility with existing Matrix
ecosystem (e.g. it's a documentation fix, or a fix in the API definition
to align with the real use), or if you implement an existing approved MSC,
you don't need to submit an MSC and it's a matter of two PRs: one to
the official repo with the spec text and API definitions, that resides at
`https://github.com/matrix-org/matrix-spec` and one to libQuotient.

If your changes require an MSC (e.g. you add a new API call or change
an existing one beyond minor adjustments):

1. Submit an MSC before submitting changes to the API definition files and
   libQuotient. See the link about MSCs above on what it should and should not
   have and how it should be submitted.
2. The MSC gets reviewed by the Spec Core Team. This can be a lengthy process
   but it's necessary for the Matrix ecosystem integrity.
3. When your MSC has at least some approvals (not necessarily a complete
   acceptance but at least some approvals should be there) the MSC process
   strongly recommends to show an implementation in existing projects. In
   the case of Client-Server API that usually means a homeserver and a client
   application. Submit PRs to the projects you took for that including
   libQuotient, referring to your MSC; for API definition files, use
   `https://github.com/quotient-im/matrix-spec` (the fork) instead of
   [the official repo](`https://github.com/matrix-org/matrix-spec`), as
   the official repo only accepts PRs on approved MSCs. You will have to show
   that your implementation is actually working to get your MSC approved.
4. Once the MSC is accepted, you can officially submit your changes in API
   definitions as a "spec PR" to the official repo.

In any case, when submitting a PR for libQuotient, please make sure generated
files are committed separately from non-generated ones (no need to make two PRs;
just separate the files in different commits).
