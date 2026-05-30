<div align=center>
<img width=150 src="https://raw.githubusercontent.com/SwirlLang/branding/main/logos/logo-transparent.png">

# Swirl Programming Language
Swirl is a statically and strongly-typed, systems programming language, leveraging the LLVM Infrastructure for optimal native code generation.

[Website](https://swirl-lang.netlify.app) |
[Docs](https://swirl-lang.netlify.app/docs) |
[Contributing](./CONTRIBUTING.md)  

[![vscode extension](https://img.shields.io/visual-studio-marketplace/v/MrinmoyHaloi.swirl-lang-support?color=blue&label=VSCode%20Extension&logo=visualstudiocode&logoColor=blue&style=flat-square)](https://marketplace.visualstudio.com/items?itemName=MrinmoyHaloi.swirl-lang-support)
[![License](https://img.shields.io/github/license/SwirlLang/Swirl?style=flat-square)](LICENSE)
[![Discord](https://img.shields.io/discord/894989427628179477?color=blue&label=Discord&logo=Discord&logoColor=white&style=flat-square)](https://discord.gg/RSJ5TUDdqx)
<!--[![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/SwirlLang/Swirl/cmake.yml?style=flat-square)](https://github.com/SwirlLang/Swirl/actions/workflows/cmake.yml)-->

</div>


## Compiler Architecture Overview
The following text is a brief on the compilation pipeline of the Compiler (details are skipped):
- [`CompilerInst`](https://github.com/SwirlLang/Swirl/blob/main/compiler/include/CompilerInst.h): the entry point, this class represents a single contained instantiation of the Compiler, owning resources which are shared across all aspects of a project's compilation.  

- [`Module`](https://github.com/SwirlLang/Swirl/blob/main/compiler/include/modules/Module.h): The compiler implements a typical module system, each Swirl file is treated as a "module" which can control the visibility of the symbols it owns (or imports) to other modules (via the `export` keyword). All modules are owned and managed by an instance of [`ModuleManager`](https://github.com/SwirlLang/Swirl/blob/main/compiler/include/modules/ModuleManager.h) which in turn is owned by `CompilerInst`.  

- [`Parser`](https://github.com/SwirlLang/Swirl/blob/main/compiler/include/parser/Parser.h): responsible for building the Abstract Syntax Tree for modules, a Parser is created for each module and invoked to build its AST, then destroyed. The Parser owns the [lexer (tokenizer)](https://github.com/SwirlLang/Swirl/tree/main/compiler/include/lexer).  

- **Sema (Semantic Analysis):** sema is a multi-pass step, consisting of the following passes:
    - [`SymbolRegistrationPass`](https://github.com/SwirlLang/Swirl/blob/main/compiler/include/sema/SymbolRegistrationPass.h): this pass registers symbols in the [`SymbolManager`](https://github.com/SwirlLang/Swirl/blob/main/compiler/include/symbols/SymbolManager.h) and resolves locally-declared symbol references into their concrete `IdentInfo*`.
    - [`SymbolResolver`](https://github.com/SwirlLang/Swirl/blob/main/compiler/include/sema/SymbolResolver.h): handles bringing imported symbols into a module's scope and resolves all globally-declared symbol references.
    - [`TypeResolver`](https://github.com/SwirlLang/Swirl/blob/main/compiler/include/sema/TypeResolver.h): this is the pass which owns the Type-System's implementation. It enforces type constrains and performs type-inference.

  **Note:** unlike parsing, Sema and the succeeding steps are performed in parallel for all modules which belong in the same "batch", a batch consists of modules which do not depend on each other, this topological sorting is done by the helper class [`ModuleManager`](https://github.com/SwirlLang/Swirl/blob/main/compiler/include/modules/ModuleManager.h) which also keeps ownership of every `Module` object.  

- If no errors were reported in the previous stage, the completion of Sema is followed by a post-sema pipeline which consists of the passes responsible for:
    -  **Compile-Time Evaluation:** evaluates and substitutes all `comptime`-marked constructs. [Resides here](https://github.com/SwirlLang/Swirl/tree/main/compiler/include/comptime).
    - **Generic Instantion:** monomorphizes generic constructs based on the generic arguments passed in their invocation. [Resides here](https://github.com/SwirlLang/Swirl/tree/main/compiler/include/generics).  

- [`LLVMBackend`](https://github.com/SwirlLang/Swirl/blob/main/compiler/include/backend/LLVMBackend.h): this class owns the codegen logic for generating the LLVM IR, since all the needed information to codegen a Module is already built in previous stages, each Module is codegen'ed in parallel.

## Contributing to Swirl
We welcome contributions to Swirl! To start contributing to Swirl, fork the repository, create a new branch, make the changes, and submit a pull request. Read the [Docs](https://swirl-lang.netlify.app/docs) for more info.

## Issues and feature request

If you want to request a new feature or report a bug, you can use the GitHub issues tracker. We will do our best to respond as quickly.
