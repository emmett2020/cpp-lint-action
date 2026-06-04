# Result Reporting

cpp-lint-action can report results in three places:

| report | default | notes |
| ------ | ------- | ----- |
| GitHub Action output | enabled | Writes `clang_format_failed_number` and `clang_tidy_failed_number`. |
| GitHub step summary | enabled | Writes the markdown summary and reproduction commands. |
| Pull request issue comment | enabled on PRs | Creates a new summary comment or updates the latest existing cpp-lint-action comment. |
| Pull request review | disabled | Sends inline comments when enabled and when comments exist. |

GitHub API reporting is only active for `pull_request` and `pull_request_target` events. For non-PR events, issue comments and pull request reviews are skipped automatically.

Inline pull request review comments are capped by `max-review-comments` and empty reviews are not posted. The summary includes notes when comments are truncated or PR API reporting is skipped.
