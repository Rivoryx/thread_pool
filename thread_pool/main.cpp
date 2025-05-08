#include <execution>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <vector>

#include "log_duration.h"
#include "thread_pool.h"

std::mutex mx;
namespace fs = std::filesystem;

void GetWholeDirInner(const fs::path& path, std::vector<fs::path>& result, ThreadPool& pool) {
	{
		// Protecting from race for result access
		std::lock_guard lock(mx);
		result.push_back(path);
	}

	if (!fs::is_directory(path)) {
		return;
	}

	// Parsing current directory
	for (const auto& dir_entry : fs::directory_iterator(path, fs::directory_options::skip_permission_denied)) {
		// Running recursion for all subdirs, using ThreadPool
		if (fs::is_directory(dir_entry)) {
			pool.Enqueue([dir_entry, &result, &pool] { 
				GetWholeDirInner(dir_entry.path(), result, pool); 
			});
		}
		else {
			std::lock_guard lock(mx);
			result.push_back(dir_entry);
		}
	}
}

std::vector<fs::path> GetWholeDir(const fs::path& path) {
	std::vector<fs::path> result;
	ThreadPool pool;
	{
		LOG_DURATION(std::string("par"));
		GetWholeDirInner(path, result, pool);
		// Waiting for all threads to finish
		pool.Wait();
	}
	std::sort(std::execution::par, std::begin(result), std::end(result));
	return result;
}

// Helper function to parse paths from different locales
static std::string to_utf8(const std::u8string& s) {
	return std::string(s.begin(), s.end());
}

// Prints all paths to a stream
template <class Container>
void Print(Container& container, std::ostream& out = std::cout) {
	for (const auto& item : container) {
		out << to_utf8(item.u8string()) << "\n";
	}
	out.flush();
}

int main() {
	fs::path path("D:\\");
	std::vector<fs::path> vector = GetWholeDir(path);
	std::ofstream of("result.txt", std::ios::binary);
	Print(vector, of);
	return 0;
}