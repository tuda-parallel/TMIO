
# How to Contribute For Students

> [!note]
> This document provides contribution guidelines for students working on their theses. Please follow these steps carefully to ensure smooth collaboration.

- [How to Contribute For Students](#how-to-contribute-for-students)
	- [Workflow Overview](#workflow-overview)
	- [Step-by-Step Instructions](#step-by-step-instructions)
		- [1. Keeping Your Branch Updated](#1-keeping-your-branch-updated)
		- [2. Committing Changes](#2-committing-changes)
		- [3. Submitting Your Work](#3-submitting-your-work)
	- [Language and Conduct](#language-and-conduct)
	- [Best Practices](#best-practices)
	- [Instructions for Adding an Example](#instructions-for-adding-an-example)
	- [Instructions for Adding a Test Case](#instructions-for-adding-a-test-case)
	- [Instructions for Adding Documentation](#instructions-for-adding-documentation)

## Workflow Overview

1. **GitHub Username**:  
   - Before starting your work, please create a GitHub account if you don’t have one already and send me your GitHub username. This will be necessary for setting up your branch and reflect your contributions to the code.

2. **Branch Creation**:  
   - Ahmad will create a branch for your work once your thesis starts. This branch will be linked to an issue that allows you to track your progress. Our meetings are reserved for content discussion. The discussions in the issue are only related to code errors.  
   - You **do not** create branches yourself. Also, **do not** work on other student branches.

3. **Creating Issues**:
   - Once your thesis starts, create an issue to describe the feature, bug fix, or enhancement you plan to implement. This helps us track contributions and avoids duplicate work. Keep the description abstract and add a few checkboxes listing what you want to add. You do not need to explicitly mention the methods. Keep it abstract, mentioning the purpose or benefits gained.
   - Go to the **Issues** tab in the [FTIO repository](https://github.com/tuda-parallel/FTIO).
   - Click **New Issue** and provide a clear title and description.
   - Label the issue appropriately as `feature` and include call it `feature...`.
   - Once you push commits, some of them should address the issue.
   - You should regularly update the issue (at least every few weeks).

4. **Development Workflow**:  
   - Work only on the branch assigned to you.  
   - Regularly pull updates from the `development` branch and merge them into your branch to stay up-to-date (at least every two weeks).
   - Build FTIO with the debug flag so that changes made in the directory are directly visible to the command line call without reinstalling FTIO. For that call:

     ```bash
     cd <ftio_repo>
     make debug #creates a virtual environment and install FTIO
     # or
     pip install -e .
     ```
   - follow the [best practice guidelines](#best-practices) to ensure code compatibility and a smooth experience for all developers

5. **Merging Restrictions**:  
   - You are **not allowed** to merge into the `development` or `main` branches.

6. **Final Submission**:  

- When your thesis is complete, create a **pull request (PR)** to merge your branch into the `development` branch.  
- Include a summary of your work and link the pull request to your issue for reference.

---

## Step-by-Step Instructions

### 1. Keeping Your Branch Updated

- Periodically update your branch with changes from `development`:

  ```bash
  git checkout development
  git pull origin development
  git checkout your-branch
  
  # Either merge or rebase with development
  git merge development
  # Or rebase for a linear history
  git rebase development
  ```

- Resolve any merge conflicts promptly and test your work.

### 2. Committing Changes

- Make frequent commits with clear and descriptive messages. Ideally, once you are finished working on an aspect, you create a commit for it.
  Example:

  ```bash
  git commit -m "FTIO: Add feature X to improve performance"
  ```

  Afterwards, push your changes from *your* branch:

  ```bash
  # You are on your-branch (check using git branch -a)
  git push
  ```
  
> [!note]
> Avoid using to short or undescriptive commit messages like 'update' or 'code cleaned'. 

### 3. Submitting Your Work

- Once your thesis is complete:  
  1. Push all changes to your branch.  
  2. Create a pull request targeting the `development` branch.  
  3. Write a description of your work, including any key contributions or challenges.

---

## Language and Conduct

1. **Appropriate Language**:  
   - Use professional, respectful, and clear language in all commit messages, comments, and documentation.  
   - Avoid using slang, jokes, or informal phrases that could be misinterpreted or deemed inappropriate.

2. **Avoid Bad Language**:  
   - Refrain from using any offensive, vulgar, or discriminatory language in any form. This applies to commit messages, comments, documentation, or communication within the team.

3. **Be Respectful**:  
   - Show courtesy when discussing issues, asking questions, or providing feedback. Collaborative communication is key to the success of the project.

4. **Constructive Feedback**:  
   - Provide helpful suggestions or feedback without criticism that could discourage others.

5. **Gender-Neutral and Inclusive Language**:  
   - Ensure that all language used in the project, including commit messages, documentation, and communication, is gender-neutral and inclusive. Avoid using gendered pronouns or assumptions, and instead use terms that are respectful and inclusive of all genders. This helps create a welcoming environment for everyone involved in the project.

---

## Best Practices

- **Stay Updated**: Regularly pull changes from `development` to avoid large merge conflicts. Also, keep the issue updated.  
- **Communicate**: Reach out if you encounter issues or need clarification.  
- **Test Thoroughly**: Ensure your work doesn’t break existing functionality. Do **not** rename or reformat entire documents, except if you created them from scratch. Regularly test your code with your [test case](/docs/students_contribute.md#instructions-for-adding-a-test-case).
- **Document Changes**: Write clear comments and update related documentation as needed.

---

## Instructions for Adding an Example

To demonstrate how to use `FTIO` with you new feature, you should add a relevant example under the `examples` directory:

1. **Create a new example script** in the `examples` folder.
2. **Ensure the example is clear**, easy to understand, and includes proper usage of `FTIO`.
3. **Push and commit** your changes:

    ```bash
    git add examples/your_example.py
    git commit -m "FTIO: Add example usage of feature XXX"
    ```

---

## Instructions for Adding a Test Case

To add a test case for verifying your changes, follow these steps:

1. **Write a new test script** in the `test` directory to check for the desired functionality of `FTIO`.
2. **Ensure the test is clear** and isolates the tested functionality.
3. **Push and commit** your changes:

    ```bash
    git add test/test_example.py
    git commit -m "Add test case for FTIO read/write functionality"
    ```

4. **Regularly test your testcase**:

    ```bash
    cd <ftio_repo>
    make test
    ```

---

## Instructions for Adding Documentation

To ensure proper documentation for your work, follow these steps:

1. **Write a new documentation file** or update an existing one in the `docs` directory.
2. **Include relevant details**, such as how to use the example, the purpose of the test cases, and any other important information.
3. **Push and commit** your changes:

    ```bash
    git add docs/example_usage.md
    git commit -m "FTIO: Add documentation for feature XXX"
    ```

4. if you made changes to the command line arguments, please update the usage section in the [readme](/README.md#usage).

---

Thanks a lot for your contribution! I look forward to seeing the progress we will make together. Let's make this a great experience! 🚀🚀
