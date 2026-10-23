/*
checkpoint_stream.hpp - Checkpointable stream wrapper
Written by Ike Yang
Generated using tools/tokenizer_py
*/

#ifndef LEXER_CHECKPOINT_STREAM_HPP
#define LEXER_CHECKPOINT_STREAM_HPP

#include <iostream>
#include <queue>

namespace lexer {
class CheckpointStream {
    std::istream& underlyingStream;
    std::queue<char> backupStream, readStream; 
    std::string savedBufferState;
    bool saving = false;
public:
    CheckpointStream(std::istream& input): underlyingStream(input) {}
    void SetCheckpoint();
    void GotoCheckpoint();
    inline bool ok() {
        return !readStream.empty() || underlyingStream.good();    
    }
    CheckpointStream& operator>>(int& c);
};

} // namespace tokenizer
#endif // TOKENIZER_CHECKPOINT_STREAM_HPP