# Contributing to Swirl

Thanks for contributing to Swirl. We want to make contributing to this project as easy and transparent as possible, whether it's:

-   Reporting a bug
-   Discussing the current state of the code
-   Submitting a fix
-   Proposing new features

## All Code Changes Happen Through Pull Requests

Pull requests are the best way to propose changes to the codebase. Do not make a pull request for adding a stuff which is incomplete or if you have not changed anything major in the source. If there are typos in identifiers or in strings, do NOT make PR's but create an issue to inform us instead. We actively welcome your pull requests:

## Process of submitting a pull request
1. Fork the repo. And [build](https://swirl-lang.vercel.app/docs/getting-started/installation#command-line) the project
1. If you've added code that should be tested, add tests.
1. If you've changed APIs, inform us about the required documentation changes needed.
1. Ensure the tests passes.
1. Make sure your code is readable.
1. Issue that pull request!

## Any contributions you make will be under the GPL v3.0 License

In short, when you submit code changes, your submissions are understood to be under the same [GPL v3.0 License](https://choosealicense.com/licenses/gpl-3.0/) that covers the project. Feel free to contact the maintainers if that's a concern.

## Report bugs using Github [issues](https://github.com/SwirlLang/Swirl/issues)

We use GitHub issues to track public bugs. Report a bug by choosing a issue template [here](https://github.com/SwirlLang/Swirl/issues/new/choose).

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

## Naming

The following conventions are being used while authoring Swirl, we request contributors to follow them too while contributing towards Swirl.

<li> PascalCase for Classes and Structures.
<li> camelCase for functions and methods.
<li> _camelCase (yes, with an underscore at the beginning) for _parameters.
<li> snake_case for local_variables.
<li> Identifier of top level constants must be in UPPER CASE.
<br>
<br>

## Commit messages guideline

We use [Conventional Commits](https://conventionalcommits.org) to make our commit messages more readable and to automate changelog generation. Please read the spec before committing to the project.

On top of the Conventional Commits, we have a few more guidelines to follow:

**Use the imperative mood in the subject line**: The subject line should describe what the commit does, rather than what has been done. For example, "Add feature X" is better than "Added feature X."

**Use bullet points to list specific changes**: If the commit includes multiple changes, it can be helpful to use bullet points to list them out. This helps to make the commit message more organized and easier to read.

**Use scopes to specify part of the codebase**: Conventional Commits allows you to specify a scope for your commit. This helps to make the commit message more organized and easier to read. The following are the scopes that we use:

* **parser**: For changes made to the parser.
* **transpiler**: For changes made to the transpiler.
* **tokenizer**: For changes made to the transpiler.
* **pre-processor**: For changes made to the preprocessor.
* **cli**: For changes made to the cli.
* **cmake**: For changes made to the CMake files.

If you can't find proper scope here, you can use the file name as the scope. For example, if you are updating the file `README.md`, you can use the scope `readme`.

If you are unsure about the scope to use, you can leave the field empty.

**Use proper spelling and grammar**: Make sure to use proper spelling and grammar in your commit messages. This helps to ensure that the messages are clear and easy to understand.

A example commit message: 
```
docs(contributing): add conventional commits to contributing guidelines

Contributing guidelines has been updated to reflect the recent changes to use Conventional Commits as spec for the commits for the project.

Closes #42
```
That's all, Thank you for having a look!
