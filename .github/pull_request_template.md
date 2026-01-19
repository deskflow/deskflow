name: Pull Request
description: pull request template
body:
  - type: markdown
    attributes:
      value: |
        Thanks for taking the time to help us improve Deskflow.

  - type: checkboxes
    id: sanity-checks
    attributes:
      label: Sanity checks
      description: |
        Please confirm the following
      options:
        - label: This PR is made by AI (LLM)
        - label: This PR is made with the help of AI (LLM)
        - label: I have built and tested the proposed changes locally
        - label: Any documentation has been updated where needed
        - label: Translations have been updated with any new strings
    validations:
      required: true
