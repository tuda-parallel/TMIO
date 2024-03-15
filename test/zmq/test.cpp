#include <iostream>
#include <zmq.hpp>
#include <msgpack.hpp>

int main() {
    zmq::context_t context(1);
	zmq::socket_t sender(context, ZMQ_PUSH);
    sender.connect("tcp://127.0.0.1:5555");
	// socket.bind("tcp://127.0.0.1:5555");

    // Create a MessagePack object to hold the data
    msgpack::sbuffer buffer;
    msgpack::packer<msgpack::sbuffer> packer(&buffer);

    // Pack the data into the MessagePack buffer
    packer.pack_map(4);
    packer.pack("ranks");
    packer.pack(8);
    // packer.pack("floatData");
    // packer.pack(3.14);

    // Pack the arrayData
    packer.pack("b");
    packer.pack_array(5);
    packer.pack(3.0);
    packer.pack(0.0);
    packer.pack(3.0);
    packer.pack(0.0);
    packer.pack(3.0);

	packer.pack("ts");
    packer.pack_array(5);
    packer.pack(1.0);
    packer.pack(2.0);
    packer.pack(3.0);
    packer.pack(4.0);
    packer.pack(5.0);

	packer.pack("te");
    packer.pack_array(5);
    packer.pack(5.0);
    packer.pack(6.0);
    packer.pack(7.0);
    packer.pack(8.0);
    packer.pack(9.0);
    zmq::message_t message(buffer.size());
    memcpy(message.data(), buffer.data(), buffer.size());

    sender.send(message, zmq::send_flags::none);

    return 0;
}



