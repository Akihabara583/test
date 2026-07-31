#include "CS2/MemoryManager.h"

MemoryManager::~MemoryManager()
{
	if (m_process)
		CloseHandle(m_process);
}

bool MemoryManager::attach_to_process(const char* window_name)
{
	if (m_process)
	{
		CloseHandle(m_process);
		m_process = nullptr;
	}

	HWND handle = FindWindowA(nullptr, window_name);
	if (!handle)
		return false;

	DWORD process_ID = 0;
	GetWindowThreadProcessId(handle, &process_ID);
	constexpr DWORD required_access =
		PROCESS_QUERY_INFORMATION | PROCESS_VM_READ |
		PROCESS_VM_WRITE | PROCESS_VM_OPERATION;
	m_process = OpenProcess(required_access, false, process_ID);
	if (!m_process)
		return false;

	Logging::log_success("Process found process ID: " + std::to_string(process_ID));

	return true;
}

bool MemoryManager::is_process_running() const
{
	if (!m_process)
		return false;

	DWORD exit_code = 0;
	return GetExitCodeProcess(m_process, &exit_code) && exit_code == STILL_ACTIVE;
}

uintptr_t MemoryManager::get_module_address(const char* module_name) const
{
	HMODULE modules[1024] = {};
	DWORD bytes_needed = 0;

	if (!EnumProcessModules(m_process, modules, sizeof(modules), &bytes_needed))
		return 0;

	const int module_count = bytes_needed / sizeof(HMODULE);
	for (size_t i = 0; i < module_count; i++)
	{
		CHAR name[MAX_PATH] = "";
		GetModuleBaseNameA(m_process, modules[i], name, sizeof(name));

		if (!strcmp(name, module_name))
		{
			if (debug_print)
			{
				std::cout << module_name << " found address: ";
				print_4_byte_hex(reinterpret_cast<uintptr_t>(modules[i]));
				std::cout << std::endl;
			}
			return reinterpret_cast<uintptr_t>(modules[i]);
		}
	}

	return 0;
}

void MemoryManager::print_4_byte_hex(uintptr_t number) const
{
	if constexpr (sizeof(uintptr_t) == 8) 
	{
		printf_s("0x%16llx", number);
	}
	else 
	{
		printf_s("0x%08llx", number);
	}
}
