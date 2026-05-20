#ifndef FILE_TRACKER_H
#define FILE_TRACKER_H

#include <filesystem>
#include <map>
#include <mutex>
#include <optional>

using PathID = size_t;

template<typename FDType, typename RequestIDType>
class [[maybe_unused]] FileTracker {
	
private:
	std::map<FDType, PathID> file_register;
	std::map<RequestIDType, PathID> request_register;
	std::mutex tracker_lock;
	std::hash<std::string> path_hasher;

public:
	PathID get_path_id(std::string& path) {
		return path_hasher(path);
	}

	void track_file_opened(const char* path, const FDType fd)
	{
		std::lock_guard lock(tracker_lock);
		// Get full unique path
		auto full_path = std::filesystem::absolute(
			std::filesystem::weakly_canonical(std::filesystem::path(path))).string();
		
		file_register.insert(std::make_pair(fd, get_path_id(full_path)));
	};

	void track_file_closed(const FDType fd) 
	{
		std::lock_guard lock(tracker_lock);
		file_register.erase(fd);
	};

	std::optional<PathID> get_fd_path(const FDType fd) 
	{
		std::lock_guard lock(tracker_lock);
		auto it = file_register.find(fd);
		if (it != file_register.end()) {
			return it->second;
		}
		return {};
	};

	bool fd_valid(const FDType fd)
	{
		std::lock_guard lock(tracker_lock);
		auto it = file_register.find(fd);
		return it != file_register.end();
	}

	void register_request(const RequestIDType request_id, const FDType fd) 
	{
		std::lock_guard lock(tracker_lock);
		auto it = file_register.find(fd);
		if (it != file_register.end()) {
			auto path = it->second;
			request_register.insert(std::make_pair(request_id, path));
		}
	};

	std::optional<PathID> get_request_path(const RequestIDType request_id) 
	{
		std::lock_guard lock(tracker_lock);
		auto it = request_register.find(request_id);
		if(it != request_register.end()) {
			return it->second;
		}
		return {};
	};

	void unregister_request(const RequestIDType request_id) 
	{
		std::lock_guard lock(tracker_lock);
		auto it = request_register.find(request_id);
		if(it != request_register.end()) {
			request_register.erase(it);		
		}
	};
};
#endif