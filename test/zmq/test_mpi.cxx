#include <iostream>
#include <zmq.hpp>
#include <msgpack.hpp>
#include <mpi.h>


void sent_data(int,double);
void sent_data_gekko(int,double);

int main(int argc, char *argv[]){

	MPI_Init(&argc, &argv);    
	
	int rank = 0, size = 0;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank); 
	
	//// simple sent
	// printf("Rank (%i/%i) sending data\n", rank, size);
	// sent_data(rank,rank);
	// printf("Rank (%i/%i) done sending\n", rank, size);

	// Gekko sent with input
	for(int i = 0; i < 3; i++){
		printf("Rank (%i/%i) sending data\n", rank, size);
		sent_data_gekko(rank,i*20);
		printf("Rank (%i/%i) done sending\n", rank, size);
		if (size == 1)
			{
			std::cout << '\n' << "Press a key to continue...";
			do 
			{} while (std::cin.get() != '\n');
			}
		else
			sleep(10);
		}
	
	
	MPI_Finalize();
    return 0;
}

void sent_data(int rank, double start = 0){
	zmq::context_t context(1);
	zmq::socket_t sender(context, ZMQ_PUSH);
    sender.connect("tcp://127.0.0.1:5555");


    // Create a MessagePack object to hold the data
    msgpack::sbuffer buffer;
    msgpack::packer<msgpack::sbuffer> packer(&buffer);

    // Pack the data into the MessagePack buffer
    packer.pack_map(4);
    packer.pack("ranks");
    packer.pack(rank);
    // packer.pack("floatData");
    // packer.pack(3.14);

    // Pack the arrayData
    packer.pack("b");
    packer.pack_array(5);
    packer.pack(300.0);
    packer.pack(0.0);
    packer.pack(300.0);
    packer.pack(0.0);
    packer.pack(300.0);

	packer.pack("ts");
    packer.pack_array(5);
    packer.pack(start + (1+std::rand())%5 + 1.0);
    packer.pack(start + (1+std::rand())%5 + 2.0);
    packer.pack(start + (1+std::rand())%5 + 3.0);
    packer.pack(start + (1+std::rand())%5 + 4.0);
    packer.pack(start + (1+std::rand())%5 + 5.0);

	packer.pack("te");
    packer.pack_array(5);
    packer.pack(start + (1+std::rand())%5 + 6.0);
    packer.pack(start + (1+std::rand())%5 + 7.0);
    packer.pack(start + (1+std::rand())%5 + 8.0);
    packer.pack(start + (1+std::rand())%5 + 9.0);
    packer.pack(start + (1+std::rand())%5 + 10.0);
    zmq::message_t message(buffer.size());
    memcpy(message.data(), buffer.data(), buffer.size());

    sender.send(message, zmq::send_flags::none);
}


void sent_data_gekko(int rank, double shift = 0){
	zmq::context_t context(1);
	zmq::socket_t sender(context, ZMQ_PUSH);
    sender.connect("tcp://127.0.0.1:5555");


    // Create a MessagePack object to hold the data
    msgpack::sbuffer buffer;
    msgpack::packer<msgpack::sbuffer> packer(&buffer);

    // Pack the data into the MessagePack buffer
    packer.pack_map(8);
    packer.pack("init_t");
    packer.pack(rank);
	packer.pack("hostname");
    packer.pack("electric");
	packer.pack("pid");
    packer.pack(0);

	packer.pack("start_t_micro");
    packer.pack_array(5);
    packer.pack(shift + (1+std::rand())%5 + 1.0);
    packer.pack(shift + (1+std::rand())%5 + 2.0);
    packer.pack(shift + (1+std::rand())%5 + 3.0);
    packer.pack(shift + (1+std::rand())%5 + 4.0);
    packer.pack(shift + (1+std::rand())%5 + 5.0);

	packer.pack("end_t_micro");
    packer.pack_array(5);
    packer.pack(shift + (1+std::rand())%5 + 6.0);
    packer.pack(shift + (1+std::rand())%5 + 7.0);
    packer.pack(shift + (1+std::rand())%5 + 8.0);
    packer.pack(shift + (1+std::rand())%5 + 9.0);
    packer.pack(shift + (1+std::rand())%5 + 10.0);
	
    packer.pack("req_size");
    packer.pack_array(5);
    packer.pack(300.0);
    packer.pack(0.0);
    packer.pack(300.0);
    packer.pack(0.0);
    packer.pack(300.0);
    
	packer.pack("total_iops");
    packer.pack(0);
	packer.pack("total_bytes");
    packer.pack(1000);

	zmq::message_t message(buffer.size());
    memcpy(message.data(), buffer.data(), buffer.size());

    sender.send(message, zmq::send_flags::none);
}


