/*
   base64.cpp and base64.h

   base64 encoding and decoding with C++.

   Version: 1.01.00

   Copyright (C) 2004-2017 René Nyffenegger
   Modified by StackOverflow User Liho
   https://stackoverflow.com/questions/180947/base64-decode-snippet-in-c/13935718#13935718
   and Tobias Kussel kussel@cbs.tu-darmstadt.de
   bloomcheck function by Sebastian Stammler and Tobias Kussel

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   René Nyffenegger rene.nyffenegger@adp-gmbh.ch

*/
#ifndef _BASE64_H_
#define _BASE64_H_

#include <string>
#include <vector>

std::string base64_encode(uint8_t const* buf, unsigned int bufLen);
std::vector<uint8_t> base64_decode(std::string const&, unsigned int);
bool check_bloom_length_and_clear_padding(std::vector<uint8_t>& bloom, const unsigned bloomlength);
std::string print_bytearray(const std::vector<uint8_t>&);
std::string print_byte(uint8_t);

#endif // _BASE64_H_

