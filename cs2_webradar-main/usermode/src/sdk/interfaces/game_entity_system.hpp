#pragma once

class c_game_entity_system
{
public:
	static constexpr size_t entities_per_chunk = 512;
	static constexpr size_t entity_entry_stride = 112;

	template <typename T = c_base_entity*>
	T get(int32_t idx)
	{
		return m_memory->read_t<T>(this->get_entity_by_idx(idx));
	}

	template <typename T = c_base_entity*>
	T get(const c_base_handle handle)
	{
		if (!handle.is_valid())
			return nullptr;

		return m_memory->read_t<T>(this->get_entity_by_idx(handle.get_entry_idx()));
	}

	template <size_t Count>
	void snapshot(std::array<c_base_entity*, Count>& entities)
	{
		entities.fill(nullptr);

		std::array<std::byte, entity_entry_stride * entities_per_chunk> chunk_data{};
		const auto chunk_count = (Count + entities_per_chunk - 1) / entities_per_chunk;

		for (size_t chunk_idx = 0; chunk_idx < chunk_count; ++chunk_idx)
		{
			const auto entry_list = m_memory->read_t<uintptr_t>(
				reinterpret_cast<uintptr_t>(this) + 8 * chunk_idx + 16);
			if (!entry_list)
				continue;

			const auto first_entity = chunk_idx * entities_per_chunk;
			const auto entities_in_chunk = std::min(entities_per_chunk, Count - first_entity);
			const auto bytes_to_read = entities_in_chunk * entity_entry_stride;
			if (!m_memory->read_t(entry_list, chunk_data.data(), bytes_to_read))
				continue;

			for (size_t idx = 0; idx < entities_in_chunk; ++idx)
			{
				uintptr_t entity_address = 0;
				std::memcpy(&entity_address, chunk_data.data() + idx * entity_entry_stride, sizeof(entity_address));
				entities[first_entity + idx] = reinterpret_cast<c_base_entity*>(entity_address);
			}
		}
	}

private:
	void* get_entity_by_idx(const int32_t idx)
	{
		if (static_cast<uint32_t>(idx) >= 0x7ffe)
			return nullptr;

		if (static_cast<uint32_t>(idx >> 9) >= 0x3f)
			return nullptr;

		const auto entry_list = m_memory->read_t<uintptr_t>(this + 8i64 * (idx >> 9) + 16);
		if (!entry_list)
			return nullptr;

		const auto player_controller = (uint32_t*)(112i64 * (idx & 0x1ff) + entry_list);
		if (!player_controller)
			return nullptr;

		return player_controller;
	}
};
