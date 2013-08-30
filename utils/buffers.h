/* buffers.h                                                       -*- C++ -*-
   Janusz Lewandowski, 30 August 2013
   Copyright (c) 2013 Daftcode.

   Released under the MIT license.
*/

#ifndef __jml__utils__buffers_h__
#define __jml__utils__buffers_h__

#include <memory>
#include <string>
#include <stdexcept>
#include "jml/compiler/compiler.h"

namespace ML {
struct buffer
{
	char* used;
	size_t size;
	size_t pos;

	JML_ALWAYS_INLINE buffer(char* used, size_t size, size_t pos = 0): used(used), size(size), pos(pos) {}
	JML_ALWAYS_INLINE operator std::string() { return {used, used + pos}; }
};

// Buffer that grows when needed.
struct growing_buffer: public buffer
{
	char internal[4096];
	std::unique_ptr<char[]> alloc;

	JML_ALWAYS_INLINE growing_buffer(): buffer(internal, 4096) {}
	JML_ALWAYS_INLINE void push_back(char c)
	{
		if (pos == size)
		{
			size_t newSize = size * 8;
			auto newBuffer = std::unique_ptr<char[]>{new char[newSize]};
			std::copy(used, used + size, newBuffer.get());
			alloc = move(newBuffer);
			used = alloc.get();
			size = newSize;
		}
		used[pos++] = c;
	}
};

// Preallocated buffer
struct external_buffer: public buffer
{
	JML_ALWAYS_INLINE external_buffer(char* external, size_t size): buffer(external, size) {}
	JML_ALWAYS_INLINE void push_back(char c)
	{
		if (pos == size)
			throw std::out_of_range{"external_buffer::push_back"};

		used[pos++] = c;
	}
};
}

#endif
