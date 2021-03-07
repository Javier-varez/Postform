# Contribution guidelines

## Types of contributions

You don't necessarily have to contribute just with pull requests. Any other kind of contributions are also really appreciated. Some examples follow here:

  * Submitting issues for feature enhancement requests or ideas.
  * Submitting bug reports.
  * Submitting design proposals to an issue or feature request.
  * Of course, contributing pull requests for open issues or new features.

## Contributing code

  * Use meaningful commit messages. Please adhere to a max. of 50 characters for the title and 72 per line for the body. You can find some more guidelines [here](https://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html) if you are interested.
  * **For Rust code**: Run cargo clippy and cargo fmt before submitting patches.
  * **For C++ code**: Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html). There are a few exceptions to these guidelines:
    * Use `m_` as a prefix for member variables instead of the `_` suffix.
    * Use `s_` as a prefix for static member variables.
    * Global variables that are not marked as static shall have a `g_` prefix.
    * Static global variables use `s_` as a prefix.
    * Use `camelCase` instead of `PascalCase` for methods.
  * C++ code should be formatted with clang-format version 10.

