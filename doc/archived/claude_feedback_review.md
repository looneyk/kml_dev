# Claude feedback review

The criticisms of Claude's work from the session of 2026-11-14, each with the memory or rule that already applies to
it and the reason that coverage failed. Nothing here is in the memories yet.

## Design and code as drafts

1. **The current code is not a constraint.** A proposed change is answered with the better design and what it would
   take to get there; a real cost is stated as a cost for the user to weigh, never as a reason the change cannot be made.
   - Instance: splitting `FlatRoomAddress` into separate ids was answered with "it can't be dropped", because today's
     per-type dispatch would break.
   - Covered by: `code.md`, "Existing interfaces, types, and helpers can change", added the same day.
   - Gap: the rule is worded for writing code, and was not applied to answering whether a change is possible.

2. **The design evolves.** It is incremental like the code, and a documented or built design is not a good reason
   against a change. A section the user writes into a document is a decision that changes the
   design and the code, and the existing plan, reference, and code are what it changes.
   - Covered by: `feedback_user_over_plan_docs` and `docs.md`, "Documents are drafts".
   - Gap: both treat the user's word as a correction to a plan that disagrees with it, not as the way the design
     evolves.

3. **The standard answer comes first.** When a framework or the industry has an established practice, it is the first
   answer, not one reached after the user pushes back.
   - Instance: "using the parent as context is ordinary in ECS", said only after two rounds of objections.
   - Covered by: `code.md`, "Follow what major open source projects do", and `writing.md`, "Standard terms come first".
   - Gap: the first applies to choosing between options already on the table, and the second to terms.

## Names

4. **A name is a judgement from its context, its usage, and the code around it, and is never built from the
   programming elements it is made of.** `ZonePlacementIndex`, an index of placements in a zone, tells the reader
   nothing about what it is used for, its significance, or what it does, and does not read correctly where it is used.
   - Covered by: `feedback_no_implementation_names` and `writing.md`, "No technology".
   - Gap: both forbid technology and data-structure words, not a name assembled from the parts of the type.

5. **A framework's conventions apply to the code written with it.** A flecs component is named for the data it
   contains, not for the entity it is attached to, so the `Zone` prefix goes.
   - Covered by: `code.md`, "Follow the conventions of the framework in use", added the same day.

6. **A general word with established meanings elsewhere is not used for a domain concept.** "Address" has many meanings
   and none of them applies; a struct containing an id is an `Id`.
   - Covered by: `writing.md`, "One reading", and "A general word that collides with a domain term is replaced".
   - Gap: the collision rule lists domain terms; "address" collides with general computing meanings instead.

7. **A nearby pattern is not a reason for a name.** The `Zone` prefix and `Address` suffix were copied from neighbors
   without checking how they read where they are used.
   - Covered by: `writing.md`, "One reading", and the `naming-review` skill.
   - Gap: the skill is run only when asked.

8. **Watch-list words keep their ordinary meaning.** "Naming" was used for adding and creating, in rules and memories.
   - Covered by: the watch list in `CLAUDE.md`, and the wording check, which now flags "named" and "naming" and checks
     memory files.

## Answers

9. **An answer includes only what was asked.** "The first sentence was useful, the rest is useless detail"; "I didn't ask
   you about the others"; "without going into minute detail as you are prone to do".
   - Covered by: `feedback_replies`, "Questions: give the answer and stop".
   - Gap: none in the wording; the memory was not followed.

10. **An objection is checked before it is raised, and dropped when it fails.** "Why on earth would a WorldspaceId be
    read as a zone id?" answered an objection that had no basis.
    - Covered by: `feedback_when_to_act`, "Vet a question before raising it".
    - Gap: it applies to questions and options, not objections.

11. **Fixing the instances found does not fix the cause that produced them, and a change says which of the two it
    does.** Renaming the `Zone` components was offered as
    removing the confusion "across the set"; it removes it across that set only, and the cause is how names are chosen.
    - Covered by: none.

12. **A question of any kind gets an answer, and work waits for a direct go-ahead.** Not doing an action the user
    expected but did not ask for is never a failure.
    - Covered by: `feedback_when_to_act`, "Act only on an explicit action verb".
    - Gap: none in the wording; the memory was not followed.

## Rules and memories

13. **A rule expresses its principle without examples, or with a minimal one.** Examples narrow the rule to their own case
    and distract from the principle.
    - Covered by: `feedback_rule_examples`, written the same day.

14. **A rule or memory goes straight to the rule.** No "the user said, asks, or wants", no "I write", and no dates or
    history.
    - Covered by: none.

15. **A rule is no stricter than the practice it describes.** "Write in plain sentences, with a subject and a verb"
    ruled out imperatives and noun phrases, both of which are often right in comments.
    - Covered by: none; `CLAUDE.md` is corrected.

16. **A rule is worded for the behavior it targets in every activity, whether answering, proposing, or editing, not
    only the activity in which the problem appeared.** The rule against treating code as fixed was in context and had
    no effect on the next answer, because it was worded for editing.
    - Covered by: none.

17. **A rule or memory is written to be understood in a later session that has none of the current session's
    context.**
    - Covered by: none.

## Trust

18. **During a cleanup, nothing changes beyond what was agreed.** A rename or any other change waits for a direct
    go-ahead, so the code under review is not a moving target.
    - Covered by: `feedback_when_to_act` and `feedback_work_units`, "One target at a time".
    - Gap: none in the wording; the memories were not followed.
