#ifndef COMMSTATEMACHINE_H
#define COMMSTATEMACHINE_H

enum CommState
{
    STATE_IDLE,
    STATE_REQUEST = 0x1,
    STATE_SEND_DATA = 0x2,
    STATE_END_SEND_DATA = 0x3,
    STATE_START_CONFIG = 0x10,
    STATE_REQUEST_TEMP = 0x12,
    STATE_REQUEST_STATUS = 0x14,
    STATE_REQUEST_VERSION = 0x15,
    STATE_REQUEST_DOWNLOAD = 0x20,
    STATE_HEADER_RECV = 0x21,
    STATE_DATA_RECV = 0x22,
};

/**
 * @file CommStateMachine.h
 * @brief Communication state machine interface and declarations.
 *
 * This file defines the communication state machine used to manage
 * protocol/state transitions for daemon socket communication.
 *
 * @author
 * @date 2026-09-17
 */
class CommStateMachine
{
public:
    explicit CommStateMachine(int fileDescriptor);
    void handleStateTransition();
private:
    void transitionToNextState();

    CommState m_currentState;
    int m_fileDescriptor; // File descriptor for the socket communication
};

#endif // COMMSTATEMACHINE_H