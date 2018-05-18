## Execution

make
./run.sh input_file schedule_alg [0 or 1] quantum size [ if schedule_alg = 1]

NB: Any input that doesn't follow the above format will lead to a segmentation fault.

## DEADLOCK DETECTION AND RECOVERY
There's no deadlock prevention implemented, the system is allowed to go into deadlock then the deadlock is detected. We recovery from the deadlock by terminating the processes involved in the deadlock one by one until the deadlock is resolved.

## OTHER

data/dp.list contains a set of requests and releases that mimic the dinning philosophers problem and deadlock the system and data/dp_sol.list contains a deadlock-free solution to the dinning philosophers problem. The solutions works by having the construction that the philosophers can only pick up the number spoons (resources) in increasing order. i.e a philosopher cannot pick spoon 5 without the possession of a lower ranked spoon.
