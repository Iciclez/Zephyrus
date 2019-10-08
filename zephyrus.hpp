#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <queue>

#ifdef _WIN64
#define X64 1
#elif _WIN32
#define X86 1
#endif

#ifdef X86
typedef uint32_t address_t;
#elif X64
typedef uint64_t address_t;
#else
typedef uint32_t address_t;
#endif


enum hook_operation : 
#ifdef X86
	uint16_t
#elif X64
	uint64_t
#endif
{
	JECXZ = 0xE3, //jmp if ecx == 0 short

	CALL = 0xE8,
	JMP = 0xE9,

	JMP_SPECIAL = 0xEA,
	JMP_SHORT = 0xEB,

	JE = 0x840F,
	JNE = 0x850F,
	JA = 0x870F,
	JB = 0x820F,

	CALL_PTR = 0x15FF,
	JMP_PTR = 0x25FF, //jmp [imm32], jmp dword ptr, jmp qword ptr

#ifdef X86
	//not in use
	CALL_64 = 0,
	JMP_64 = 0
#elif X64
	CALL_64 = 0x08EB0000000215FF,
	JMP_64 = 0xffffffff000025FF, //bytes for jmp imm64 is 0x0000000025FF
#endif


	//0x70 <= pb[0] && pb[0] <= 0x7F
	// jo, jno, jb, jnb, jz, jnz, jbe, ja, js, jns, jp, jnp, jl, jnl, jle, jnle


	//(pb[0] == 0x0F && (0x80 <= pb[1] && pb[1] <= 0x8F))
	//
};


class zephyrus
{
public:
	enum padding_byte : uint8_t
	{
		NOP = 0x90,
		INT3 = 0xCC,
	};

	zephyrus(padding_byte padding = NOP);
	~zephyrus() noexcept;

	bool pagereadwriteaccess(address_t address);
	uint32_t protectvirtualmemory(address_t address, size_t size);

	const std::vector<uint8_t> readmemory(address_t address, size_t size);
	bool writememory(address_t address, const std::string &array_of_bytes, size_t padding_size = 0, bool retain_bytes = true);
	bool writememory(address_t address, const std::vector<uint8_t> &bytes, bool retain_bytes = true);
	bool copymemory(address_t address, void *bytes, size_t size, bool retain_bytes = true);
	bool writeassembler(address_t address, const std::string &assembler_code, bool retain_bytes = true);
	bool writepadding(address_t address, size_t padding_size);
	bool revertmemory(address_t address);

	bool redirect(hook_operation operation, address_t *address, address_t function, bool enable = true);
	bool redirect(address_t *address, address_t function, bool enable = true);
	template <class T> bool redirect(T *address, T function, bool enable = true);
	template <class T> bool redirect(hook_operation operation, T *address, T function, bool enable = true);

	bool detour(void **from, void *to, bool enable = true);
	template <class T> bool detour(T *from, T to, bool enable = true);

	bool sethook(hook_operation operation, address_t address, address_t function, size_t nop_count = -1, bool retain_bytes = true);
	bool sethook(hook_operation operation, address_t address, const std::string &assembler_code, size_t nop_count = -1, bool retain_bytes = true);
	bool sethook(hook_operation operation, address_t address, const std::vector<std::string> &assembler_code, size_t nop_count = -1, bool retain_bytes = true);

	template <hook_operation T> bool sethook(address_t address, address_t function, size_t nop_count = -1, bool retain_bytes = true);
	template <hook_operation T> bool sethook(address_t address, const std::string &assembler_code, size_t nop_count = -1, bool retain_bytes = true);
	template <hook_operation T> bool sethook(address_t address, const std::vector<std::string> &assembler_code, size_t nop_count = -1, bool retain_bytes = true);

	template <typename T> bool writedata(address_t address, T data);
	template <typename T> const T readdata(address_t address);
	template <typename T> static bool writepointer(address_t base, size_t offset, T value);
	template <typename T> static const T readpointer(address_t base, size_t offset);
	template <typename T> static bool writemultilevelpointer(address_t base, std::queue<size_t> offsets, T value);
	template <typename T> static const T readmultilevelpointer(address_t base, std::queue<size_t> offsets);

	//
	bool assemble(const std::string &assembler_code, _Out_ std::vector<uint8_t> &bytecode);

	size_t getnopcount(address_t address, hook_operation operation);
	
	static const std::string byte_to_string(const std::vector<uint8_t> &bytes, const std::string &separator = " ");
	static const std::vector<uint8_t> string_to_bytes(const std::string &array_of_bytes);

	template <typename T> static T convert_to(const std::vector<uint8_t> &bytes);

	static address_t getexportedfunctionaddress(const std::string &module_name, const std::string &function_name);
	template <typename T> static T getexportedfunction(const std::string &module_name, const std::string &function_name);

private:
	padding_byte padding;
	std::unordered_map<address_t, std::vector<uint8_t>> memoryedit;

	std::function<bool(address_t, size_t, const std::function<void(void)> &)> pageexecutereadwrite;
	std::unordered_map<address_t, std::vector<uint8_t>> hook_memory;

	std::unordered_map<address_t, std::pair<address_t, address_t>> trampoline_detour;
	std::unordered_map<address_t, std::vector<uint8_t>> trampoline_table;
};

template<class T>
inline bool zephyrus::redirect(T * address, T function, bool enable)
{
	return this->redirect(reinterpret_cast<address_t*>(address), reinterpret_cast<address_t>(function), enable);
}

template<class T>
inline bool zephyrus::redirect(hook_operation operation, T * address, T function, bool enable)
{
	return this->redirect(operation, reinterpret_cast<address_t*>(address), reinterpret_cast<address_t>(function), enable);
}

template<class T>
inline bool zephyrus::detour(T * from, T to, bool enable)
{
	return this->detour(reinterpret_cast<void**>(from), to, enable);
}

template<hook_operation T>
inline bool zephyrus::sethook(address_t address, address_t function, size_t nop_count, bool retain_bytes)
{
	return this->sethook(T, address, function, nop_count, retain_bytes);
}

template<hook_operation T>
inline bool zephyrus::sethook(address_t address, const std::string & assembler_code, size_t nop_count, bool retain_bytes)
{
	return this->sethook(T, address, assembler_code, nop_count, retain_bytes);
}

template<hook_operation T>
inline bool zephyrus::sethook(address_t address, const std::vector<std::string>& assembler_code, size_t nop_count, bool retain_bytes)
{
	return this->sethook(T, address, assembler_code, nop_count, retain_bytes);
}

template<typename T>
inline bool zephyrus::writedata(address_t address, T data)
{
	return this->pageexecutereadwrite(address, sizeof(T), [&]()
	{
		*reinterpret_cast<T*>(address) = data;
	});
}

template<typename T>
inline const T zephyrus::readdata(address_t address)
{
	return zephyrus::convert_to<T>(this->readmemory(address, sizeof(T)));
}

template<typename T>
inline bool zephyrus::writepointer(address_t base, size_t offset, T value)
{
	if (!base)
	{
		return false;
	}

	__try
	{
		*reinterpret_cast<T*>(*reinterpret_cast<address_t*>(base) + offset) = value;
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
}

template<typename T>
inline const T zephyrus::readpointer(address_t base, size_t offset)
{
	__try
	{
		return base ? *reinterpret_cast<T*>(*reinterpret_cast<address_t*>(base) + offset) : 0;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return 0;
	}
}

template<typename T>
inline bool zephyrus::writemultilevelpointer(address_t base, std::queue<size_t> offsets, T value)
{
	if (!base)
	{
		return false;
	}


	for (base = *reinterpret_cast<address_t*>(base); !offsets.empty(); offsets.pop())
	{
		if (offsets.size() == 1)
		{
			*reinterpret_cast<T*>(base + offsets.front()) = value;
			return true;
		}
		else
		{
			//the for loop deref our base 
			base = *reinterpret_cast<address_t*>(base + offsets.front());
		}
	}

	return false;

}

template<typename T>
inline const T zephyrus::readmultilevelpointer(address_t base, std::queue<size_t> offsets)
{
	if (!base)
	{
		return 0;
	}

	for (base = *reinterpret_cast<address_t*>(base); !offsets.empty(); offsets.pop())
	{
		if (offsets.size() == 1)
		{
			return *reinterpret_cast<T*>(base + offsets.front());
		}
		else
		{
			//the for loop deref our base 
			base = *reinterpret_cast<address_t*>(base + offsets.front());
		}
	}

	return 0;
}

template<typename T>
inline T zephyrus::convert_to(const std::vector<uint8_t>& bytes)
{
	std::vector<uint8_t> b = bytes;

	if (sizeof(T) > b.size())
	{
		b.insert(b.end(), sizeof(T) - b.size(), 0);
	}

	T m = 0;
	for (int32_t n = sizeof(m) - 1; n >= 0; --n)
	{
		m = (m << 8) + b.at(n);
	}

	return m;
}

template<typename T>
inline T zephyrus::getexportedfunction(const std::string & module_name, const std::string & function_name)
{
	return reinterpret_cast<T>(getexportedfunctionaddress(module_name, function_name));
}
