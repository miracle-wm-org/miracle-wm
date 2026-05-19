/**
Copyright (C) 2025  Matthew Kosarek

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#include "sampler_registry.h"

uint8_t miracle::SamplerRegistry::register_sample_to_rgba(std::string sample_to_rgba_func)
{
    std::lock_guard lock { mutex };
    auto const next_id = id++;
    entries.push_back(Entry { next_id, std::move(sample_to_rgba_func) });
    return next_id;
}
