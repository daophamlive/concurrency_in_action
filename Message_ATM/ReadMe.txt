--send object
sender has a reference to the queue
send message type T to queue

--receiver object
receiver has a queue
cast to sender by creating a sender with reference to this queue
wait function moves the reference of queue to depatcher

--dispatcher
constructor creates get a pointer queue and make chain is false to wait and dispatcher
handle creates a template_dispatcher with the queue, itself and forward a function

wait and dispatch: wait a message input into queue and dispatch it
dispatch : dynamic cast to WRAPPED_MESSAGE<CLOSE_QUEUE> if true throw a exception

--template_dispatcher

dispatch : dynamic cast to WRAPPED_MESSAGE<MSG> then calling the function with content
if return false call prev dispatch

-- atm object
 - bank receiver
 - interface hardware receiver
 - atm receiver
 
-create a sender with atm receiver
 