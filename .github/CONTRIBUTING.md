# Contributing to DirectZ

Thank you for considering contributing to DirectZ! This engine is built with performance, clarity, and cross-platform support in mind. Whether you're fixing bugs, adding features, or improving documentation, your input is appreciated.

## Getting Started

1. Fork the repository
2. Clone your fork: `git clone https://github.com/your-username/DirectZ.git`
3. Create a branch: `git checkout -b branch-name`

## Build Instructions

See [README.md](/README.md)

## Coding Guidelines

- DirectZ standard is set to C++20, stick to this
- Avoid unnecessary abbreviations
- Group related logic together and comment only when it adds clarity

## Commit Standards

Use clear and scoped commit messages, such as:

`[Asset] Add support for streaming KTX textures`

`[Core] Fix crash when recreating Vulkan swapchain`

## Submitting Changes

1. Make sure your branch is up to date with master
2. Test your changes thoroughly on supported platforms
    - you can use the branch `run-actions` to trigger a global build on all platforms
    - or use `x-actions` (i.e. `windows-actions`) to trigger build on a specific platform
3. Open a pull request with a detailed description
4. Reference related issues if relevant

## Need Help?

If you have questions, open an issue with the "question" label or email: [directz+support@atomicmail.io](mailto:directz+support@atomicmail.io)

---

Let's make DirectZ stronger together.