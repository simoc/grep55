
#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdlib.h>
#include <cctype>

#ifdef __GNUC__

#if __GNUC__ < 8
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

/**
 * \brief Parses settings from INI file in home directory
 */
class Settings
{
public:
	Settings(const std::string &name)
	{
		const std::vector<std::string> default_settings =
		{
			"[files]",
			"suffixes=txt log xml html c h cpp cs java bat sh sql"
		};

		std::vector<std::string> settings_lines;
		char *home = getenv("HOME");
		if (!home)
			home = getenv("USERPROFILE");
		if (home)
		{
			// Open settings file in home directory
			fs::path home_dir = home;
			std::string filename = name + ".ini";
			fs::path settings_file = home_dir / filename;

			std::ifstream in(settings_file);
			if (!in.is_open())
			{
				std::ofstream out(settings_file);
				if (out.is_open())
				{
					for (auto line : default_settings)
					{
						out << line << std::endl;
					}
				}
				out.close();
				in.open(settings_file);
			}
			if (in.is_open())
			{
				// Read all lines from settings file
				while (!in.eof())
				{
					std::string line;
					std::getline(in, line);
					settings_lines.push_back(line);
				}
			}
			else
			{
				// Could not create settings file, use defaults anyway
				settings_lines = default_settings;
			}
		}
		else
		{
			// Could not determine home directory, use defaults anyway
			settings_lines = default_settings;
		}

		// Parse lines in settings file
		std::string section;
		for (const std::string &line : settings_lines)
		{
			std::string trimmed = ltrim(line);
			if (trimmed.find("[") == 0)
			{
				size_t pos = trimmed.find("]");
				if (pos != std::string::npos)
				{
					section = trimmed.substr(1, pos - 1);
				}
			}
			else
			{
				size_t pos = trimmed.find("=");
				if (pos != std::string::npos)
				{
					std::string key = trimmed.substr(0, pos);
					std::string value = trimmed.substr(pos + 1);
					process(section, key, value);
				}
			}
		}
	}

	bool is_valid_suffix(const std::string &suffix) const
	{
		std::string suffix2 = tolowercase(suffix);
		if (suffix2.find(".") == 0)
			suffix2 = suffix2.substr(1);

		bool found = std::find(m_suffixes.begin(),
			m_suffixes.end(), suffix2) != m_suffixes.end();
		return found;
	}

private:
	std::vector<std::string> m_suffixes;

	std::string tolowercase(const std::string &str) const
	{
		std::string lowercase;
		std::transform(str.begin(), str.end(), std::back_inserter(lowercase),
			[](unsigned char c){ return std::tolower(c); });
		return lowercase;
	}

	std::string ltrim(const std::string &str) const
	{
		size_t i = str.find_first_not_of(" \t");
		if (i == std::string::npos)
			return str;
		else
			return str.substr(i);
	}

	void process(const std::string &section,
		const std::string &key, const std::string &value)
	{
		if (section == "files" && key == "suffixes")
		{
			std::istringstream ss(value);
			while(!ss.eof())
			{
				std::string suffix;
				ss >> suffix;
				suffix = tolowercase(suffix);
				m_suffixes.push_back(suffix);
			}
		}
	}
};

/**
 * \brief Recursively searches files for lines containing a pattern
 */
class RecursiveSearcher
{
public:
	void search(const fs::path path,
		const std::string &pattern,
		int level,
		const Settings &settings)
	{
		if (fs::is_directory(path))
		{
			std::vector<fs::path> filenames;
			for (const fs::directory_entry &entry : fs::directory_iterator(path, fs::directory_options::skip_permission_denied))
			{
				filenames.push_back(entry.path());
			}
			// Ensure predictable order of files
			std::sort(filenames.begin(), filenames.end());
			for (const fs::path &filename : filenames)
			{
				search(filename, pattern, level + 1, settings);
			}
		}
		else if (level == 0 || fs::is_regular_file(path))
		{
			if (level > 0 &&
				settings.is_valid_suffix(path.extension().string()) == false)
			{
				return;
			}

			std::ifstream in(path);
			if (in.is_open())
			{
				int line_number = 0;
				while (!in.eof())
				{
					std::string line;
					std::getline(in, line);
					line_number++;

					if (line.find(pattern) != std::string::npos)
					{
						std::cout << path.string() << ":" <<
							line_number << ": " <<
							line <<
							std::endl;
					}
				}
			}
			else
			{
				std::cerr << strerror(errno) << ": " << path.string() << std::endl;
			}
		}
	}
};

int
main(int argc, char *argv[])
{
	fs::path program = argv[0];
	Settings settings(program.stem().string());

	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " pattern [filename] ..." << std::endl;
		return 1;
	}

	std::string pattern = argv[1];
	std::vector<fs::path> filenames;
	for (int i = 2; i < argc; i++)
	{
		fs::path filename = argv[i];
		filenames.push_back(filename);
	}
	if (filenames.empty())
	{
		fs::path filename = fs::current_path();
		filenames.push_back(filename);
	}
	RecursiveSearcher r;
	for (const fs::path &filename : filenames)
	{
		r.search(filename, pattern, 0, settings);
	}
	return 0;
}
