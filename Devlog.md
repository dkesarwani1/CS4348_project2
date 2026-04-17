## 2026 March 22nd 9:40 PM
Objective: Implement a real-world banking system simulation using C++ multithreading and semaphores for synchronization, with proper handling of shared resources. The key synchronization relationships are: customer ↔ teller, teller ↔ manager (withdrawals only), and teller ↔ safe. There will be 3 teller threads and 50 customer threads.

Design decisions made:
- counting_semaphore<2> door(2) — limits bank entry to 2 customers at a time, simulating a physical door constraint
- counting_semaphore<1> manager(1) — only 1 manager available, required for withdrawal approvals
- counting_semaphore<2> safe(2) — safe allows up to 2 tellers at a time
- counting_semaphore<50> customerready(0) — initialized to 0; tellers block on this until a customer signals they are ready
- queue<int> customerQueue protected by queueMutex — FIFO queue of customer IDs waiting to be served
- atomic<bool> bankClosed — flag so tellers can detect when all customers have been served and exit cleanly

Progress: Declared all global semaphores, mutexes, and the customer queue. Laid out the thread structure — 3 teller threads and 50 customer threads to be created in main.

## 2026 March 23rd 9:05 PM
Objective: Implement the core teller workflow including transaction type handling, manager approval for withdrawals, and safe access synchronization.

Teller workflow implemented:
1. Print "ready to serve", then block on customerready.acquire() waiting for a customer
2. Check bankClosed flag — if true, exit the loop immediately
3. Pop the next customer ID from customerQueue under queueMutex
4. Simulate service time with sleep_for(rand() % 2000ms)
5. Randomly decide transaction type: Deposit or Withdrawal via rand() % 2
6. If Withdrawal: acquire manager semaphore, simulate manager approval (5–30ms delay), then release it
7. Acquire safe semaphore, simulate safe access (10–50ms), release safe
8. Increment servedCustomers under servedlock; if count reaches 50, set bankClosed = true and break

Also added int servedCustomers with a dedicated mutex servedlock so the served count can be safely incremented across teller threads without a data race.

Progress: Teller function fully implemented with correct semaphore ordering. Transaction type selection, manager gating for withdrawals, and safe access are all working. The teller owns the full transaction flow end-to-end before signaling the customer, preventing a race where a customer could leave before the transaction is fully recorded.

## 2026 March 29th 10:50 PM
Objective: Implement the customer function and connect that to the teller workflow.
Next Steps:
- Add a vector of per-customer binary semaphores (customerDone) so each customer can block until a teller finishes serving them.
- Implement customer function funtion where it acquire door, push to queue, signal customerready, wait on customerDone[id], release door.
- In teller function, after safe transaction completes, signal customerDone[customerId] to let the customer leave.
- Fix the main function so it can initialize customerDone semaphores, fix tellers.push_back() typo, and join all threads before returning.

## 2026 March 30th 9:00 PM
- Added vector of per-customer binary semaphores (customerDone) initialized in main.
- Implemented customer function: acquires door, pushes to queue, signals customerready, waits on customerDone[id], releases door.
- Teller signals customerDone[customerId] after safe transaction completes.
- Fixed main: semaphores initialized, tellers.push_back() typo fixed, all threads joined before return with cleanup of allocated semaphores.
Progress: Implementation is complete. Next step is testing and verification.

Things Noticed: There were issues with the original output in that two threads were writing to cout at the same time which caused major issues and I had to just fixed it to by adding coutMutex so only a single thread can write to cout one at a time. Then ensures more consistency within the output. Another thing is I Improved the labeling of the tellers from 0-2 to 1-3 for consistency.

## 2026 April 17th 12:10AM
Went back over the code I thought I finished before submitting and found the code had drifted from the assignment in several places. It ran without deadlocking, which is why I missed these earlier.
Issues found:

No bank-open barrier — customers could enter before all 3 tellers were ready.
Teller was picking the transaction type; per the spec the customer should decide it.
No real handshake — teller did the whole transaction alone, skipping the "ask for transaction / give transaction" back-and-forth.
Output didn't match the required TYPE ID [TYPE ID]: MSG format.
Customer arrival was 0–999 ms instead of 0–100 ms, and there were two extra 2000 ms sleeps in the teller that aren't in the spec.
Teller IDs were 1–3 but customers were 0–49; spec example uses Teller 0.
"Ready to serve" printed on every loop iteration instead of once at startup.
Used rand() across threads (not thread-safe).
Shutdown released customerready twice without a clean chain — off-by-one risk.

## 2026 April 17th 12:50AM
Fixes Made from before:
- Bank-open barrier: Added a condition_variable called bankCV and a bankIsOpen flag. All customer threads now block on bankCV.wait() until all 3 tellers have called ready to serve and the count reaches 3. The last teller to become ready sets bankIsOpen = true and calls notify_all(), releasing all waiting customers simultaneously.

- Customer decides transaction: Moved transaction type selection (Deposit or Withdrawal) to the very first line of the customer function, before the 0–100 ms wait. It is written into transactionType[id] and later read by the teller only after proper semaphore synchronization.

- Full teller-customer handshake: Added three sets of per-teller binary_semaphore arrays — tellerAskSem, customerTellSem, and customerLeftSem. The teller now signals the customer to ask for the transaction, the customer responds with the type, and the teller waits for the customer to leave before looping back. Also added customerAssigned[id] per-customer semaphores so the customer knows which teller to interact with.

- Output format: Rewrote all print statements to follow TYPE ID [TYPE ID]: MSG. Added helper functions logLine and logResource to keep formatting consistent across all log sites. Also added the additional required lines: performing transaction and transaction in safe complete inside the safe block, and waiting for manager approval and manager approval complete inside the manager block.

- Timing: Fixed customer arrival to rand() % 101 (0–100 ms). Removed the two 2000 ms teller sleeps that were not in the spec.

- Ready to serve placement: Moved the ready-to-serve print to once at the top of the teller function, before the loop, as the spec requires.

- Shutdown: Fixed to release customerready exactly NUM_TELLERS - 1 times (2) so each remaining blocked teller gets exactly one wake-up signal and sees bankClosed = true.

-Door logging: Added waiting to use door before door.acquire() and changed exit message to leaves bank, encapsulated in a passThroughDoor() helper used for both entry and exit.

## Reflection

The initial design got the semaphore structure right — door(2), manager(1), safe(2) — but the protocol was wrong. The teller owned the entire transaction when the spec clearly puts the customer in charge of deciding and communicating the transaction type. Fixing that required the full tellerAskSem/customerTellSem/customerLeftSem handshake and was the most significant change to the project.

The main lesson was that "runs without deadlocking" isint the same as "follows the spec." I thought the program finished for weeks before a proper line-by-line analysis was done and I found multiple issues. Starting from the required output format and working backward to the semaphore design would have caught most of them on day one.
