#include "Connection.h"

#include <iostream>

namespace Afina {
namespace Network {
namespace STnonblock {

// See Connection.h
void Connection::Start() {
    _event.events |= EPOLLIN;
}

// See Connection.h
void Connection::OnError() {
    _isAlive_flag = false;
}

// See Connection.h
void Connection::OnClose() {
    _isAlive_flag = false;
}

// See Connection.h
void Connection::DoRead() {
    assert(_write_only==false);
    try {
        readed_bytes = read(_socket, &client_buffer[0] + read_offset, sizeof(client_buffer) - read_offset);
        if(readed_bytes > 0) {
            _logger->debug("Got {} bytes from socket", readed_bytes);

            while (readed_bytes > 0) {
                _logger->debug("Process {} bytes", readed_bytes);
                // There is no command yet
                if (!command_to_execute) {
                    std::size_t parsed = 0;
                    if (parser.Parse(client_buffer, readed_bytes, parsed)) {
                        // There is no command to be launched, continue to parse input stream
                        // Here we are, current chunk finished some command, process it
                        _logger->debug("Found new command: {} in {} bytes", parser.Name(), parsed);
                        command_to_execute = parser.Build(arg_remains);
                        if (arg_remains > 0) {
                            arg_remains += 2;
                        }
                    }

                    // Parsed might fails to consume any bytes from input stream. In real life that could happens,
                    // for example, because we are working with UTF-16 chars and only 1 byte left in stream
                    if (parsed == 0) {
                        read_offset += readed_bytes;
                        break;
                    } else {
                        std::memmove(client_buffer, client_buffer + parsed, readed_bytes - parsed);
                        readed_bytes -= parsed;
                    }
                }

                // There is command, but we still wait for argument to arrive...
                if (command_to_execute && arg_remains > 0) {
                    _logger->debug("Fill argument: {} bytes of {}", readed_bytes, arg_remains);
                    // There is some parsed command, and now we are reading argument
                    std::size_t to_read = std::min(arg_remains, std::size_t(readed_bytes));
                    argument_for_command.append(client_buffer, to_read);

                    std::memmove(client_buffer, client_buffer + to_read, readed_bytes - to_read);
                    arg_remains -= to_read;
                    readed_bytes -= to_read;
                }

                // There is command & argument - RUN!
                if (command_to_execute && arg_remains == 0) {
                    _logger->debug("Start command execution");

                    std::string result;
                    if (argument_for_command.size()) {
                        argument_for_command.resize(argument_for_command.size() - 2);
                    }
                    command_to_execute->Execute(*pStorage, argument_for_command, result);

                    // Send response
                    result += "\r\n";


                    _logger->debug("Result is ready: {}", result);

                    if (output_buffer.empty()) {
                        _event.events |= EPOLLOUT;
                    }

                    output_buffer.push_back(result);
                    output_iovec.emplace_back();
                    output_iovec[output_iovec.size()-1].iov_base =  &output_buffer.back()[0];
                    output_iovec[output_iovec.size()-1].iov_len =  result.size();

                    _logger->debug("Add to iovec {} byte string", output_buffer.back().size());


                    if (output_buffer.size() > 100) {
                        _event.events &= ~EPOLLIN;
                    }

                    // Prepare for the next command
                    read_offset = 0;
                    command_to_execute.reset();
                    argument_for_command.resize(0);
                    parser.Reset();
                }
            }

        }else if (readed_bytes == 0) {
            _logger->debug("Connection closed: readed_bytes = 0");
            _event.events &= ~EPOLLIN;
            if(output_buffer.empty()){
                _isAlive_flag = false;
            }else{
                _write_only = true;
            }

        } else {
            throw std::runtime_error(std::string(strerror(errno)));
        }

    } catch (std::runtime_error &ex) {
        _logger->error("Failed to process connection on descriptor {}: {}", _socket, ex.what());
        _isAlive_flag = false;
    }
}


// See Connection.h
void Connection::DoWrite() {
    int ret;

    output_iovec[0].iov_base += head_offset;
    output_iovec[0].iov_len -= head_offset;

    ret = writev(_socket, &output_iovec[0], output_iovec.size());

    _logger->debug("Writev {} bytes to socket, queue lenght {}", ret, output_iovec.size());

    if(ret <= 0){
        _isAlive_flag = false;
    }else{
        auto it1 = output_buffer.begin();

        //_logger->debug("Iovec base: {}, len:  {}", *(static_cast<std::string* >((*it2).iov_base)), (*it2).iov_len);

        size_t front_element_size = (*it1).size() - head_offset;

        while(ret >= front_element_size){
            ret -= front_element_size;

            output_buffer.pop_front();
            output_iovec.pop_front();

            if(!output_buffer.empty()){
                it1 = output_buffer.begin();

                front_element_size = (*it1).size();
            }else{
                break;
            }

        }
        head_offset = ret;
    }

    _logger->debug("After writing {} elems left in queue", output_buffer.size());

    if (output_buffer.size() < 90 && !_write_only) {
        _event.events |= EPOLLIN;
    }

    if (output_buffer.empty()) {
        _event.events &= ~EPOLLOUT;
        if(_write_only){
            _isAlive_flag = false;
        }
    }


}

} // namespace STnonblock
} // namespace Network
} // namespace Afina
