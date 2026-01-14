
#include <cstdlib>
#include <cstring>

#include "siv3d_spine_extension.h"


void* CS3dSpineExtension::_alloc(size_t size, [[maybe_unused]] const char* file, [[maybe_unused]] int line)
{
	if (size == 0) return nullptr;

	return std::malloc(size);
}

void* CS3dSpineExtension::_calloc(size_t size, [[maybe_unused]] const char* file, [[maybe_unused]] int line)
{
	if (size == 0) return nullptr;

	void* p = std::malloc(size);
	if (p != nullptr)
	{
		std::memset(p, 0, size);
	}

	return p;
}

void* CS3dSpineExtension::_realloc(void* ptr, size_t size, [[maybe_unused]] const char* file, [[maybe_unused]] int line)
{
	if (size == 0) return nullptr;

	void* p = nullptr;
	if (ptr == nullptr)
	{
		p = std::malloc(size);
	}
	else
	{
		p = std::realloc(ptr, size);
	}

	return p;
}

void CS3dSpineExtension::_free(void* mem, [[maybe_unused]] const char* file, [[maybe_unused]] int line)
{
	std::free(mem);
}

char* CS3dSpineExtension::_readFile([[maybe_unused]] const spine::String& path, [[maybe_unused]] int* length)
{
	/* ファイルアクセスはSiv3DのAPIを通じて行い、その後メモリ上のデータを用いてSpineのAPIを呼び出すので、この関数は実装しない。*/
	return nullptr;
}

spine::SpineExtension* spine::getDefaultExtension()
{
	return new CS3dSpineExtension();
}
