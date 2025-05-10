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
static std::atomic<int> files = 0;
static std::atomic<int> dirs = 0;

// Helper function to parse paths from different locales
static std::string from_utf8(const std::u8string& s) {
	return std::string(s.begin(), s.end());
}

void GetWholeDirInner(const fs::path& path, std::vector<fs::path>& result, ThreadPool& pool) {
	{
		// Protecting from race for result access
		std::lock_guard lock(mx);
		result.push_back(path);
	}
	++dirs;

	//Creating temp vector to insert all files with a single insertion later
	std::vector<fs::path> subdir_files;

	std::error_code ec;
	// Parsing current directory
	for (const auto& dir_entry : fs::directory_iterator(path, fs::directory_options::skip_permission_denied, ec)) {

		if (ec) {
			std::lock_guard lock(mx);
			std::cerr << "Skip entry in " << from_utf8(dir_entry.path().u8string())
				<< ": " << ec.message() << std::endl;
			ec.clear();
			continue;
		}

		// Skip symlinks to avoid infinite recursion in Linux
		if (fs::is_symlink(dir_entry)) {
			continue;
		}

		// Running recursion for all subdirs, using ThreadPool
		if (fs::is_directory(dir_entry)) {
			pool.Enqueue([dir_entry, &result, &pool] {
				GetWholeDirInner(dir_entry.path(), result, pool);
				});
		}
		else {
			++files;
			subdir_files.push_back(dir_entry.path());
		}
	}

	{
		std::lock_guard lock(mx);
		result.insert(std::end(result), std::begin(subdir_files), std::end(subdir_files));
	}
}

std::vector<fs::path> GetWholeDir(const fs::path& path) {
	if (!fs::exists(path)) {
		return {};
	}
	if (!fs::is_directory(path)) {
		++files;
		return { path };
	}
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

// Prints all paths to a stream
template <class Container>
void Print(Container& container, std::ostream& out = std::cout) {
	for (const auto& item : container) {
		out << from_utf8(item.u8string()) << "\n";
	}
	out.flush();
}

int main() {
	fs::path path("C:\\Windows\\");
	std::cout << "Start with " << from_utf8(path.u8string()) << std::endl;
	std::vector<fs::path> vector = GetWholeDir(path);
	std::ofstream of("result.txt", std::ios::binary);
	Print(vector, of);
	double files_percentage = (files + dirs == 0) ? 0 : static_cast<double>(files) * 100 / (files + dirs);
	std::cout
		<< "For path: \"" << from_utf8(path.u8string())
		<< "\" dirs: " << dirs << " files: " << files
		<< " files percentage is: " << files_percentage
		<< "%" << std::endl;
	return 0;
}
