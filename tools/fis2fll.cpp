/**
 * \file fis2fll.cpp
 *
 * \brief Convert a MATLAB's FIS file into the corresponding Fuzzylite's FLL format.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 *
 * <hr/>
 *
 * Copyright 2016 Marco Guazzone (marco.guazzone@gmail.com)
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

#include <fl/Headers.h>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cerr << "Insufficient number of arguments" << std::endl;
		std::cerr << "Usage: " << argv[0] << " <FIS file> [<FLL file>]" << std::endl;
		return 1;
	}

	std::string fis_file(argv[1]);
	std::string fll_file;
	if (argc > 2)
	{
		fll_file = argv[2];
	}
	else
	{
		if (fis_file.size() >= 4 && fis_file.substr(fis_file.size()-4) == ".fis")
		{
			fll_file = fis_file.substr(0, fis_file.size()-4) + ".fll";
		}
		else
		{
			fll_file = fis_file + ".fll";
		}
	}

	fl::Engine* p_eng = 0;

	fl::fuzzylite::setDecimals(std::numeric_limits<fl::scalar>::digits10+1);
	fl::fuzzylite::setMachEps(std::numeric_limits<fl::scalar>::epsilon());

	fl::FisImporter fisImp;
	p_eng = fisImp.fromFile(fis_file);

	fl::FllExporter fllExp;
	fllExp.toFile(fll_file, p_eng);

	delete p_eng;
	fisImp.fromFile(fis_file);
}
