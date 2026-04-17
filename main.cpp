#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <string>
#include <semaphore>
#include <cstdlib>
#include <ctime>

using namespace std;

constexpr int NUM_TELLERS = 3;
constexpr int NUM_CUSTOMERS = 50;

// Doorway capacity: only two customers may pass through the door at once.
counting_semaphore<2> doorSem(2);
counting_semaphore<1> managerSem(1);
counting_semaphore<2> safeSem(2);
counting_semaphore<NUM_CUSTOMERS> customerReady(0);

mutex bankMutex;
condition_variable bankCV;
bool bankIsOpen = false;
int tellerReadyCount = 0;

mutex coutMutex;
mutex queueMutex;
mutex servedMutex;

int servedCustomers = 0;
atomic<bool> bankClosed(false);
queue<int> customerQueue;

binary_semaphore* customerAssigned[NUM_CUSTOMERS];
binary_semaphore* customerDone[NUM_CUSTOMERS];

binary_semaphore* tellerAskSem[NUM_TELLERS];
binary_semaphore* customerTellSem[NUM_TELLERS];
binary_semaphore* customerLeftSem[NUM_TELLERS];

int assignedTeller[NUM_CUSTOMERS];
string transactionType[NUM_CUSTOMERS];

void logLine(const string& actorType, int actorId,
             const string& targetType, int targetId,
             const string& message)
{
    lock_guard<mutex> lk(coutMutex);
    cout << actorType << ' ' << actorId << " [" << targetType << ' ' << targetId << "]: "
         << message << '\n';
}

void logResource(const string& actorType, int actorId,
                 const string& resource,
                 const string& message)
{
    lock_guard<mutex> lk(coutMutex);
    cout << actorType << ' ' << actorId << " [" << resource << "]: "
         << message << '\n';
}

void passThroughDoor(int customerId, const string& actionMessage)
{
    logLine("Customer", customerId, "Customer", customerId, "waiting to use door");
    doorSem.acquire();
    logLine("Customer", customerId, "Customer", customerId, actionMessage);
    this_thread::sleep_for(chrono::milliseconds(1));
    doorSem.release();
}

void customer(int id)
{
    transactionType[id] = (rand() % 2 == 0) ? "Deposit" : "Withdrawal";
    logLine("Customer", id, "Customer", id,
            "wants to make a " + transactionType[id]);

    logLine("Customer", id, "Customer", id, "waiting to go to bank");
    this_thread::sleep_for(chrono::milliseconds(rand() % 101));
    logLine("Customer", id, "Customer", id, "going to bank");

    {
        unique_lock<mutex> lk(bankMutex);
        bankCV.wait(lk, [] { return bankIsOpen; });
    }

    passThroughDoor(id, "enters bank");

    logLine("Customer", id, "Customer", id, "gets in line");
    {
        lock_guard<mutex> lk(queueMutex);
        customerQueue.push(id);
    }
    customerReady.release();

    customerAssigned[id]->acquire();
    int myTeller = assignedTeller[id];
    logLine("Customer", id, "Teller", myTeller, "selects teller");

    logLine("Customer", id, "Teller", myTeller, "introduces self");

    logLine("Customer", id, "Teller", myTeller,
            "waiting for teller to ask for transaction");
    tellerAskSem[myTeller - 1]->acquire();

    logLine("Customer", id, "Teller", myTeller,
            "tells teller " + transactionType[id]);
    customerTellSem[myTeller - 1]->release();

    logLine("Customer", id, "Teller", myTeller,
            "waiting for transaction to complete");
    customerDone[id]->acquire();
    logLine("Customer", id, "Teller", myTeller, "transaction complete");

    passThroughDoor(id, "leaves bank");
    customerLeftSem[myTeller - 1]->release();
}

void teller(int id)
{
    logLine("Teller", id, "Teller", id, "ready to serve");

    bool iAmLast = false;
    {
        lock_guard<mutex> lk(bankMutex);
        ++tellerReadyCount;
        if (tellerReadyCount == NUM_TELLERS) {
            bankIsOpen = true;
            bankCV.notify_all();
            iAmLast = true;
        }
    }

    if (iAmLast) {
        logLine("Teller", id, "Teller", id, "bank is now open");
    }

    while (true) {
        logLine("Teller", id, "Teller", id, "waiting for customer");
        customerReady.acquire();

        if (bankClosed) {
            break;
        }

        int customerId;
        {
            lock_guard<mutex> lk(queueMutex);
            if (customerQueue.empty()) {
                continue;
            }
            customerId = customerQueue.front();
            customerQueue.pop();
        }

        assignedTeller[customerId] = id;
        customerAssigned[customerId]->release();

        logLine("Teller", id, "Customer", customerId, "serving customer");

        logLine("Teller", id, "Customer", customerId, "asks for transaction type");
        tellerAskSem[id - 1]->release();

        customerTellSem[id - 1]->acquire();
        const string txn = transactionType[customerId];
        logLine("Teller", id, "Customer", customerId,
                "received " + txn + " request");

        if (txn == "Withdrawal") {
            logResource("Teller", id, "Manager", "going to manager");
            managerSem.acquire();
            logResource("Teller", id, "Manager", "with manager");
            logResource("Teller", id, "Manager", "waiting for manager approval");
            this_thread::sleep_for(chrono::milliseconds(rand() % 26 + 5));
            logResource("Teller", id, "Manager", "manager approval complete");
            logResource("Teller", id, "Manager", "done with manager");
            managerSem.release();
        }

        logResource("Teller", id, "Safe", "going to safe");
        safeSem.acquire();
        logResource("Teller", id, "Safe", "in safe");
        logResource("Teller", id, "Safe", "performing transaction");
        this_thread::sleep_for(chrono::milliseconds(rand() % 41 + 10));
        logResource("Teller", id, "Safe", "transaction in safe complete");
        logResource("Teller", id, "Safe", "done in safe");
        safeSem.release();

        logLine("Teller", id, "Customer", customerId,
                "transaction complete, notifying customer");
        customerDone[customerId]->release();

        logLine("Teller", id, "Customer", customerId,
                "waiting for customer to leave");
        customerLeftSem[id - 1]->acquire();
        logLine("Teller", id, "Teller", id, "customer has left");

        bool allDone = false;
        {
            lock_guard<mutex> lk(servedMutex);
            ++servedCustomers;
            if (servedCustomers == NUM_CUSTOMERS) {
                bankClosed = true;
                allDone = true;
            }
        }

        if (allDone) {
            logLine("Teller", id, "Teller", id,
                    "all customers served, bank closing");
            for (int i = 0; i < NUM_TELLERS - 1; ++i) {
                customerReady.release();
            }
            break;
        }
    }
}

int main()
{
    srand(static_cast<unsigned>(time(nullptr)));

    for (int i = 0; i < NUM_CUSTOMERS; ++i) {
        customerAssigned[i] = new binary_semaphore(0);
        customerDone[i] = new binary_semaphore(0);
    }

    for (int i = 0; i < NUM_TELLERS; ++i) {
        tellerAskSem[i] = new binary_semaphore(0);
        customerTellSem[i] = new binary_semaphore(0);
        customerLeftSem[i] = new binary_semaphore(0);
    }

    vector<thread> tellers;
    vector<thread> customers;

    for (int i = 1; i <= NUM_TELLERS; ++i) {
        tellers.emplace_back(teller, i);
    }
    for (int i = 0; i < NUM_CUSTOMERS; ++i) {
        customers.emplace_back(customer, i);
    }

    for (auto& c : customers) {
        c.join();
    }
    for (auto& t : tellers) {
        t.join();
    }

    for (int i = 0; i < NUM_CUSTOMERS; ++i) {
        delete customerAssigned[i];
        delete customerDone[i];
    }
    for (int i = 0; i < NUM_TELLERS; ++i) {
        delete tellerAskSem[i];
        delete customerTellSem[i];
        delete customerLeftSem[i];
    }

    return 0;
}
