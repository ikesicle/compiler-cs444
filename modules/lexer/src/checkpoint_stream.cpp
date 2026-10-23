#include "lexer/checkpoint_stream.hpp"

namespace lexer {

void CheckpointStream::SetCheckpoint() {
    while (!backupStream.empty()) {
        backupStream.pop();
    }
}

void CheckpointStream::GotoCheckpoint() {
    while (!readStream.empty()) {
        backupStream.push(readStream.front());
        readStream.pop();
    }
    while (!backupStream.empty()) {
        readStream.push(backupStream.front());
        backupStream.pop();
    }
}

CheckpointStream& CheckpointStream::operator>>(int& c) {
    char cv;
    if (!readStream.empty()) {
        cv = readStream.front();
        readStream.pop();
        c = cv;
        backupStream.push(cv);
    } else if (underlyingStream >> cv) {
        c = cv;
        backupStream.push(cv);
    } else {
        c = EOF;
    }
    // std::cerr << "Readstream length: " << readStream.size() << ", backupstream length: " << backupStream.size() << "\n";
    return *this;
}

} // namespace lexer