enum Commands{
    // IR commands 
    NEXT_PKT = 0b11100000,
    REPEAT = 0b00011111,
    CHUNK_NUMBER = 0b1,
    START = 0b00000000,
    STOP = 0b11111111,
    // Intern commands which are not transmitted via IR
    VALID_MESSAGE = 0b10101010,
    CMD_NOT_FOUND_ERROR = 404,
};

enum SuperState{
    NONE,
    SENDER,
    RECEIVER
};

enum TransmissionState {
    WAITING_FOR_PERMISSION,
    PERMISSION_GRANTED,
    PERMISSION_EXPIRED,
};