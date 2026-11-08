---
name: github-feature-planning
description: Plan implementation-ready software features and create one detailed GitHub issue for each feature. Use when asked to break down a roadmap, enhancement, migration, or technical task into GitHub issues with scope, acceptance criteria, implementation guidance, and tests.
compatibility: Requires git, GitHub CLI (gh), and authenticated GitHub access with permission to create issues.
---

# GitHub feature planning

Turn a requested body of work into small, independently reviewable features,
then create one GitHub issue per feature.  An issue is an implementation
contract, not a vague reminder.

## Workflow

1. **Establish repository context and the mandatory issue target.**
   - Read the repository's existing documentation, architecture, conventions,
     active branch, and relevant source files.
   - GitHub issues for this skill always belong to the repository configured as
     the local Git remote named `origin`. Never create, edit, close, or inspect
     planning issues in `upstream` or in any repository other than `origin`.
     This rule has no exception.
   - Derive and validate the GitHub owner/repository from `origin`; do not use
     GitHub CLI's automatic repository selection:
     ```sh
     origin_url=$(git remote get-url origin) || exit 1
     origin_repo=${origin_url#git@github.com:}
     origin_repo=${origin_repo#https://github.com/}
     origin_repo=${origin_repo#ssh://git@github.com/}
     origin_repo=${origin_repo%.git}
     case "$origin_repo" in
       */*) ;;
       *) echo "origin is not a supported GitHub repository URL: $origin_url" >&2; exit 1 ;;
     esac
     gh repo view "$origin_repo"
     ```
   - If `origin` is absent, is not a GitHub URL, resolves ambiguously, or the
     user intends another repository, stop and ask the user to correct the
     `origin` remote. Do not fall back to `upstream`.
   - Verify access with `gh auth status`.
   - Inspect open issues only in `origin` before creating anything:
     ```sh
     gh issue list --repo "$origin_repo" --state open --limit 100
     ```
   - Do not create a duplicate.  If an existing issue substantially covers a
     feature, report it and propose updating it instead of opening a new one.

2. **Define feature boundaries.**
   - Split work by user-visible capability or independently deployable
     technical outcome, not merely by file or implementation phase.
   - Keep cross-cutting prerequisites explicit.  Create a separate issue when
     a prerequisite can be implemented and reviewed independently.
   - State dependencies using issue references after issues exist.
   - If requirements are materially ambiguous, ask focused questions before
     creating issues.  Otherwise make bounded assumptions and record them in
     the issue.

3. **Write complete issue drafts to temporary files.**
   - Create one temporary directory for the plan, for example:
     ```sh
     plan_dir=$(mktemp -d "${TMPDIR:-/tmp}/pi-feature-plan.XXXXXX")
     ```
   - Write each complete Markdown body once to a numbered file such as
     `$plan_dir/01-riscv-libc-selection.md`. Do not keep a second, manually
     reproduced copy in the conversation or command history.
   - Record the issue title, file path, order, and SHA-256 checksum in a small
     manifest in the same directory. The file is the single source of truth
     for the draft that the user approves and that will be uploaded.
   - Use this structure for every issue. Omit a section only when it is truly
   inapplicable, and say why where that could otherwise be surprising.

   ```markdown
   ## Summary

   <One concise statement of the delivered capability.>

   ## Context and problem

   <Why this is needed, current behaviour, relevant constraints, and links to
   source files, documentation, or related issues.>

   ## Scope

   ### In scope
   - ...

   ### Out of scope
   - ...

   ## Implementation guidance

   - <Concrete relevant paths, interfaces, build configuration, compatibility
     requirements, migration considerations, and design constraints.>
   - <Describe the intended approach without prescribing unsupported details.>

   ## Acceptance criteria

   - [ ] <Observable functional result>
   - [ ] <Failure mode or edge case, where applicable>
   - [ ] <Compatibility, performance, security, or documentation requirement>

   ## Test plan

   - [ ] <Exact automated test, command, or test location to add/run>
   - [ ] <Manual or hardware validation, including expected observation>
   - [ ] <Regression coverage for affected existing behaviour>

   ## Dependencies and follow-up

   - Depends on: ...
   - Follow-up: ...
   ```

4. **Present the exact files for interactive approval before GitHub upload.**
   - Do not create any GitHub issue immediately after drafting it.
   - Display the complete proposed issue plan in the Pi conversation: the
     mandatory `origin` target repository, numbered titles, intended dependency
     order, file names, checksums, and the full Markdown content read from each
     temporary draft file.
   - Ask the user to choose one of: **approve upload**, **request changes**,
     or **cancel**. Accept targeted feedback by issue number as well as
     feedback that changes the overall split, scope, acceptance criteria, or
     test plan.
   - For requested changes, edit only the affected temporary draft files,
     update their manifest checksums, display the updated file contents, and
     ask again. Repeat until the user explicitly approves upload or cancels.
   - Treat the conversation as the interactive Pi TUI element. A skill cannot
     add native clickable widgets; no GitHub side effect is allowed until the
     user replies with explicit approval.

5. **Make technical issues actionable.**
   For build systems, toolchains, firmware, APIs, migrations, or other
   technical work, include where known:
   - affected files and ownership boundaries;
   - configuration inputs and generated outputs;
   - compatibility matrix and unsupported combinations;
   - error handling and diagnostics;
   - test environments, fixtures, hardware, simulators, or CI commands;
   - documentation that must be updated.

   Never invent test results, source paths, hardware behaviour, or upstream
   constraints. Mark unknown items as investigation or acceptance work.

6. **Create issues and report results.**
   Create issues only after the user has explicitly approved the displayed
   draft plan.

   - Use English for titles and bodies unless the repository explicitly uses
     another language.
   - Every GitHub command must include `--repo "$origin_repo"`.
   - Before every upload, verify that the draft file still matches its approved
     manifest checksum. If it does not, stop and display the changed file for
     a new approval; never regenerate or silently rewrite an approved body.
   - Create each issue directly from its approved file:
     ```sh
     gh issue create --repo "$origin_repo" --title "$title" --body-file "$draft_file"
     ```
     Do not construct a second heredoc, interpolate body text in a shell
     command, or regenerate the body after approval.
   - Add only labels that already exist. Do not create labels, milestones,
     projects, or assignees unless requested.
   - Create issues in dependency order. To keep approved files immutable,
     describe dependencies in drafts by stable draft number and title (for
     example, `Depends on: Draft 1 — Add ...`), not a future GitHub number.
     Do not edit an uploaded issue merely to replace that text with a newly
     assigned number unless the user explicitly approves the exact edit.
   - Report each issue number, URL, title, dependency relation, and any
     assumptions or blocked questions.

## Quality gate

Before creating an issue, verify that another developer can answer all of
these from its body:

- What outcome is being delivered and why?
- What is explicitly included and excluded?
- Which source areas or interfaces are likely affected?
- How will acceptance be observed?
- Which automated, manual, simulator, or hardware tests are required?
- What preconditions, dependencies, and compatibility constraints apply?

If an issue cannot answer these questions, improve it before creating it.

## Default behaviour and safety

Create issues only when the request asks to create them. For a request to
plan, review, or draft only, present the proposed issue set without external
side effects. Never close, edit, label, assign, or delete existing issues
unless explicitly requested. All GitHub issue operations must target the local
`origin` repository explicitly; never use GitHub CLI's implicit repository
selection and never operate on `upstream`. After approval, upload only the
approved temporary draft files; a changed file always requires a new approval.
