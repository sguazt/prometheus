/* vim: set tabstop=4 expandtab shiftwidth=4 softtabstop=4: */

/**
 * \file fuzzylite_version.cpp
 *
 * \brief Retrieve information about fuzzylite version
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright 2021 Marco Guazzone (marco.guazzone@gmail.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <dcs/string/algorithm/split.hpp>
#include <fl/Headers.h>
#include <iostream>
#include <vector>


int main(int argc, char *argv[])
{
	bool major = false;
	bool minor = false;
	bool patch = false;

	if (argc < 2)
	{
		major = minor = patch = true;
	}
	else
	{
		for (int arg = 1; arg < argc; ++arg)
		{
			if (!std::strcmp(argv[arg], "major"))
			{
				major = true;
			}
			else if (!std::strcmp(argv[arg], "minor"))
			{
				minor = true;
			}
			else if (!std::strcmp(argv[arg], "patch"))
			{
				patch = true;
			}
		}
	}

	std::vector<std::string> parts = dcs::string::split(fl::fuzzylite::version(), '.');
    if (major && parts.size() > 0)
    {
        std::cout << parts[0] << " ";
    }
    if (minor && parts.size() > 1)
    {
        std::cout << parts[1] << " ";
    }
    if (patch && parts.size() > 2)
    {
        std::cout << parts[2] << " ";
    }
    std::cout << "\n";
}
