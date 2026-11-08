---
name: github-issue-implementation
description: Interactively select open GitHub issues and implement approved features one at a time, pausing after each feature for acceptance or rework. Use when asked to choose, implement, review, or iterate on repository features tracked as GitHub issues.
compatibility: Requires git, GitHub CLI (gh), and authenticated GitHub access. Interactive selection uses the Pi conversation; no custom TUI extension is required.
---

# Interactive GitHub issue implementation

Implement only issues that the user explicitly selects and authorizes. Work on
selected issues sequentially. After each implementation, stop and wait for the
user to accept it or request rework before starting another issue.

## 1. Discover and present open issues

1. Confirm repository context and GitHub access:
   ```sh
   git status --short --branch
   git remote -v
   gh auth status
   ```
2. Fetch open issues with enough information for a selection:
   ```sh
   gh issue list --state open --limit 100 \
     --json number,title,labels,assignees,url,body
   ```
3. Read every candidate issue body before presenting it. Exclude pull requests
   and identify duplicates, blockers, and already-satisfied issues.
4. Present a concise numbered selection in the Pi conversation. For each
   issue, show:
   - issue number and title;
   - labels, if any;
   - one-sentence summary;
   - known dependencies or blockers;
   - URL.
5. Ask the user to select one or more issue numbers and explicitly authorize
   implementation. Accept selections such as `#42`, `42, 51`, or `all except
   19`. Do not begin implementation until the user has selected issues.

A skill cannot add a native clickable Pi TUI widget. The numbered selection
and the user's next chat response are the interactive selection mechanism.

## 2. Prepare one selected issue

Process selected issues in dependency order. Before editing code for each
issue:

1. Read its complete GitHub body and all linked, blocking, and prerequisite
   issues that affect its implementation.
2. Inspect relevant source files, tests, documentation, generated files, and
   project conventions.
3. Check the working tree. Do not overwrite or discard unrelated user changes.
4. Restate the implementation plan concisely, including affected paths,
   acceptance criteria, and tests. If the issue is ambiguous, blocked, or
   materially inconsistent with the repository, stop and ask the user rather
   than guessing.

## 3. Implement and verify

For the current issue only:

1. Make narrow, reviewable changes that satisfy its stated scope.
2. Preserve backward compatibility unless the issue explicitly changes it.
3. Add or update focused tests for each observable behaviour. For technical
   work, run the relevant build, unit, integration, simulator, hardware, or
   static-analysis checks that are available.
4. Update English project documentation when behaviour, configuration,
   developer workflow, or compatibility changes.
5. Inspect the final diff and report:
   - files changed and why;
   - commands run and their results;
   - acceptance criteria satisfied;
   - tests not run, unavailable environments, assumptions, or remaining
     hardware/manual validation.

Never claim that a test, build, board, device, or external service was
validated unless it actually was.

## 4. Mandatory acceptance pause

After completing and reporting one issue, stop. Do not start the next selected
issue and do not make further implementation edits until the user chooses one
of the following:

- **Accept** — implementation is accepted; retain the working-tree changes and
  proceed to the next selected issue, if any.
- **Rework** — ask for or use the user's feedback, revise only the current
  issue, rerun affected checks, and present a new acceptance report.
- **Stop** — leave all changes intact and end the implementation session.
- **Accept and close** — after the user explicitly requests this, post a short
  GitHub issue comment with the verification summary and close the issue using
  `gh issue close`.

An acceptance decision is never inferred from silence, a successful test, or a
clean diff. Do not close, label, assign, or otherwise edit a GitHub issue
without an explicit user instruction.

## 5. Continue or finish

When the user accepts a feature and more selected issues remain, announce the
next issue and repeat the preparation workflow. If all selected issues are
accepted, provide a final summary with each issue number, status, changed
paths, and outstanding validation.

## Safety and quality rules

- Never implement an unselected issue.
- Never mix unrelated features in one implementation pass.
- Do not commit, push, create pull requests, close issues, or alter issue
  metadata unless the user explicitly requests that action.
- Preserve all pre-existing uncommitted changes.
- Follow the repository's language policy: project documentation and GitHub
  issue comments are English unless the repository explicitly specifies
  otherwise.
