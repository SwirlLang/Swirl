# Contributing to Swirl

Thanks for contributing to Swirl. We want to make contributing to this project as easy and transparent as possible, whether it's:

-   Reporting a bug
-   Discussing the current state of the code
-   Submitting a fix
-   Proposing new features

## All Code Changes Happen Through Pull Requests

Pull requests are the best way to propose changes to the codebase. Do not make a pull request for adding a stuff which is incomplete or if you have not changed anything major in the source. If there are typos in identifiers or in strings, do NOT make PR's but create an issue to inform us instead. We actively welcome your pull requests:

## Process of submitting a pull request
1. Fork the repo. And [build](#building-the-project) the project
1. If you've added code that should be tested, add tests.
1. If you've changed APIs, update the documentation.
1. Ensure the tests passes.
1. Make sure your code is readable.
1. Issue that pull request!

## Building the project
The project uses CMake as the build tool. So make sure you have CMake installed on your system.<br>
After cloning the repo cd into it and follow the steps below.
### Configure CMake
```bash
cmake -B build - DCMAKE_BUILD_TYPE=Debug -S Swirl
```

### Build the project
```bash
cmake --build build --config Debug
```
### Run the app
```bash
./build/swirl
```
## Any contributions you make will be under the GPL v3.0 License

In short, when you submit code changes, your submissions are understood to be under the same [GPL v3.0 License](https://choosealicense.com/licenses/gpl-3.0/) that covers the project. Feel free to contact the maintainers if that's a concern.

## Report bugs using Github's [issues](https://github.com/SwirlLang/Swirl/issues)

We use GitHub issues to track public bugs. Report a bug by choosing a issue template [here](https://github.com/SwirlLang/Swirl/issues/new/choose). it's that easy!

## Write bug reports with detail, background, and sample code

[This is an example](http://stackoverflow.com/q/12488905/180626) of a bug report.

**Great Bug Reports** tend to have:

-   A quick summary and/or background
-   Steps to reproduce
    -   Be specific!
    -   Give sample code if you can. Code that _anyone_ can run to reproduce what you were seeing.
-   What you expected would happen
-   What actually happens
-   Notes (possibly including why you think this might be happening, or stuff you tried that didn't work)

People _love_ thorough bug reports.

## License

By contributing, you agree that your contributions will be licensed under GPL v3.0 License.

# Conventions

The following conventions are being used while authoring Swirl, we request contributors to follow them too while contributing towards Swirl.

<li> PascalCase for Classes and Structures.
<li> camelCase for functions and methods.
<li> _PascalCase (with an underscore at the beginning) for _Parameters.
<li> snake_case for local_variables.
<li> Identifier of top level constants must be in UPPER CASE.
<br>
<br>
That's all, Thank you for having a look!