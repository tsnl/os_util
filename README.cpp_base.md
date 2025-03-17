# `cpp_base`

This repo is a skeleton for C++ projects.

Other repos should start by forking this project.

Updates to common files can then be inherited via rebase.

## Setup Guide

In your new repo, run:

```bash
$ git remote add cpp_base_origin https://github.com/tsnl/cpp_base
$ git fetch cpp_base_origin
$ git checkout cpp_base_main
```

Now, you can create your main branch by running:

```bash
$ git checkout -b main
```

You can apply any `cpp_base` updates to your `main` branch by rebasing on `cpp_base_main`:

```bash
$ get fetch cpp_base_origin
$ git checkout cpp_base_main
$ git pull
$ git checkout main
$ git rebase cpp_base_main
```
