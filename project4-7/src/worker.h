#pragma once

#include <mr_task_factory.h>
#include "mr_tasks.h"
#include "masterworker.grpc.pb.h"
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <grpcpp/grpcpp.h>
#include <algorithm>
#include <grpc/support/log.h>


using namespace std;

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using masterworker::TaskRequest;
using masterworker::TaskResponse;
using masterworker::FilePath;

/* CS6210_TASK: Handle all the task a Worker is supposed to do.
	This is a big task for this project, will test your understanding of map reduce */
class Worker {

	public:
		/* DON'T change the function signature of this constructor */
		Worker(std::string ip_addr_port);
		
		//Starting
		void handleRequests();
	
		/* DON'T change this function's signature */
		bool run();

	private:
		/* NOW you can add below, data members and member functions as per the need of your implementation*/
	class RequestHandler {
	   public:
	    RequestHandler(masterworker::Worker::AsyncService* service, ServerCompletionQueue* cq, Worker* worker)
	        : service(service), worker_request_cq(cq), response_writer(&worker_server_context), finish(false), worker(worker) {
	        service->RequestassignTask(&worker_server_context, &request, &response_writer, worker_request_cq, worker_request_cq,
	                                    this);
	    }

	    void processRequest() {
				string str;
				vector<string> file_paths;
	      if (!finish) {
	        new RequestHandler(service, worker_request_cq, worker);

					if(request.task_type() == "MAP") {

						auto mapper = get_mapper_from_task_factory("cs6210");
						mapper->impl_->num_reducers = request.num_reducers();
						mapper->impl_->output_dir = request.output_dir();

						for(int i=0; i<request.file_paths().size(); i++) {
							string file_abs_path = request.file_paths(i).file_path();
							int start_offset = request.file_paths(i).start_offset();
							int end_offset = request.file_paths(i).end_offset();
							std::ifstream file(file_abs_path, std::ios::in);
							if(!file.is_open()){
								cout << "Unable to open file\n";
							} else {
								file.seekg (start_offset, file.beg);

								while(file.tellg() <= end_offset && file.tellg()!=-1) {
									std::getline(file, str);
									mapper->map(str);
								}
							}

							file.close();
						}

						mapper->impl_->flush();
						file_paths = mapper->impl_->getFilePaths();
						for(auto entry: file_paths) {
							FilePath* file_path = response.add_file_paths();
							file_path->set_file_path(entry);
						}

						response.set_status(1);
					} else {

						map<string, vector<string>> file_data;

						auto reducer = get_reducer_from_task_factory("cs6210");
						reducer->impl_->output_dir = request.output_dir();
						int index = request.file_paths(0).file_path().find('.');
						reducer->impl_->output_file = "final" + request.file_paths(0).file_path().substr(index-1, index);
						for(auto entry: request.file_paths()) {
								string intermediate_file_path = entry.file_path();

								ifstream intermediate_file(intermediate_file_path, ios::in);

								if(!intermediate_file.is_open()){
									cout << "Unable to open intermediate file\n";
								} else {
									while(getline(intermediate_file, str)) {
										string key = str.substr(0, str.find(' '));
										string value = str.substr(str.find(' ')+1, str.length());

										file_data[key].push_back(value);

									}


								}
							}
							for(auto x: file_data) {
								reducer->reduce(x.first, x.second);
							}
							response.set_status(1);
					}
	        finish = true;
	        response_writer.Finish(response, Status::OK, this);

	      } else {
	        delete this;
	      }
	    }

	   private:
	    masterworker::Worker::AsyncService* service;
	    ServerCompletionQueue* worker_request_cq;
	    ServerContext worker_server_context;
			TaskRequest request;
			TaskResponse response;
	    Worker* worker;

	    ServerAsyncResponseWriter<TaskResponse> response_writer;
	    bool finish;
	  };

		string ip_addr_port;
		masterworker::Worker::AsyncService service;
		std::unique_ptr<Server>  server;
		std::unique_ptr<ServerCompletionQueue> worker_queue_cq;
};


/* CS6210_TASK: ip_addr_port is the only information you get when started.
	You can populate your other class data members here if you want */
Worker::Worker(std::string ip_addr_port) {
	ip_addr_port = ip_addr_port;
	ServerBuilder builder;
	// Listen on the given address without any authentication mechanism.
	builder.AddListeningPort(ip_addr_port, grpc::InsecureServerCredentials());
	// Register "service" as the instance through which we'll communicate with
	// clients. In this case it corresponds to an *synchronous* service.
	builder.RegisterService(&service);
	worker_queue_cq = builder.AddCompletionQueue();
	// Finally assemble the server.
	server=builder.BuildAndStart();
	std::cout << "Worker running on " << ip_addr_port << std::endl;
}

extern std::shared_ptr<BaseMapper> get_mapper_from_task_factory(const std::string& user_id);
extern std::shared_ptr<BaseReducer> get_reducer_from_task_factory(const std::string& user_id);


void Worker::handleRequests() {
    // Spawn a new CallData instance to serve new clients.
    new RequestHandler(&service, worker_queue_cq.get(), this);
    void* tag;  // uniquely identifies a request.
    bool ok;
    while (true) {
      GPR_ASSERT(worker_queue_cq->Next(&tag, &ok));
      GPR_ASSERT(ok);
      static_cast<RequestHandler*>(tag)->processRequest();
    }
}





/* CS6210_TASK: Here you go. once this function is called your woker's job is to keep looking for new tasks 
	from Master, complete when given one and again keep looking for the next one.
	Note that you have the access to BaseMapper's member BaseMapperInternal impl_ and 
	BaseReduer's member BaseReducerInternal impl_ directly, 
	so you can manipulate them however you want when running map/reduce tasks*/
bool Worker::run() {
	/*  Below 5 lines are just examples of how you will call map and reduce
		Remove them once you start writing your own logic */ 
	std::cout << "worker.run(), I 'm not ready yet" <<std::endl;
	auto mapper = get_mapper_from_task_factory("cs6210");
	mapper->map("I m just a 'dummy', a \"dummy line\"");
	auto reducer = get_reducer_from_task_factory("cs6210");
	reducer->reduce("dummy", std::vector<std::string>({"1", "1"}));

	//-------------------------
	handleRequests();
	return true;
}
