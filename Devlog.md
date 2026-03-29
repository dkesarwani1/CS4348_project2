## 2026 March 22nd 9:40 PM
Objective: The obejctive of the project is to implement real world banking system where using multiple threads and semaphores for synchronization. Also requiring Proper handling of the shared resources. its important to maintain a synchronization between teller and customer. And also a synchronization between teller and manager as well as also teller and safe. There will be 3 tellers and 50 customers. Door limit will be 2 per entry for customers. and a teller entry of 2. 
Progress: Added the customer and teller relation as well as the semaphores and queue.

## 2026 March 23rd 9:05 PM
Today I will try to do the basic implementation of the teller and the customer functions. Inside the teller, I will implement the teller workflow.
Progress: Added a lot of the teller relation with adding all the transactions types and also added a count variable where it checks how many customers the bank serves. Additionally, I added all the relations to where the teller serves the customer all the way through till the teller finishes the task of the customer.

## 2026 March 29th 10:50 PM
Objective: Implement the customer function and connect it to the teller workflow.
Next Steps:
- Add a vector of per-customer binary semaphores (customerDone) so each customer can block until a teller finishes serving them.
- Implement customer function funtion where it acquire door, push to queue, signal customerready, wait on customerDone[id], release door.
- In teller function, after safe transaction completes, signal customerDone[customerId] to let the customer leave.
- Fix the main function so it can initialize customerDone semaphores, fix tellers.push_back() typo, and join all threads before returning.