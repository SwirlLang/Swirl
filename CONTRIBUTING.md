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

Here are the guidelines for writing effective commit messages:

**Keep the subject line short and concise**: The subject line should be a brief summary of the changes made in the commit. It should be no more than 50 characters long, and it should not end with a period.

**Use the imperative mood in the subject line**: The subject line should describe what the commit does, rather than what has been done. For example, "Add feature X" is better than "Added feature X."

**Use the body to provide context and details**: The body of the commit message should provide more detailed information about the changes made in the commit. It should explain the motivations behind the changes, and it should provide any relevant context or background information.

**Separate the subject line and body with a blank line**: Use a blank line to separate the subject line from the body of the commit message. This helps to visually distinguish between the two sections of the message.

**Use bullet points to list specific changes**: If the commit includes multiple changes, it can be helpful to use bullet points to list them out. This helps to make the commit message more organized and easier to read.

**Use tags to highlight important information**: The tags should be used to reflect the name of the file or component updated. Like cli, parser, transpiler. If there are multiple components, files updated, use a comma to separate the tags and descriptions. For example, "cli, parser: Update help message, Implement loop parser.".

Here is a list of common tags:
* **cli**: For changes made to the cli.
* **parser**: For changes made to the parser.
* **transpiler**: For changes made to the transpiler.
* **core**: For changes made to the core.
* **pre-processor**: For changes made to the preprocessor.
* **cmake**: For changes made to the CMake files.
* **packaging**: For changes made to the packaging system.

If you can't find your tag here, you can use the file name as the tag. For example, if you are updating the file `README.md`, you can use the tag `readme`.

If you are unsure about the tag to use, you can use the `misc` tag.

**Use proper spelling and grammar**: Make sure to use proper spelling and grammar in your commit messages. This helps to ensure that the messages are clear and easy to understand.

A example commit message: 
```
cli: Update include guard

Shifted include guard location after the include statements. And changed include guard format from SWIRL_CLI_H to CLI_H_Swirl.
```
That's all, Thank you for having a look!
