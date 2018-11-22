/*
*   Copyright (C) 2018 Paul Bernitz
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*	
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <random>
#include <algorithm>
#include <functional>

#include <SFML/Graphics.hpp>

/*-----------------------------------.
| A single entity of the population. |
`-----------------------------------*/
struct Person
{
	bool is_updated;
	bool active;
	sf::Color color;
	bool is_male;
	float disease;
	float reproduction;
	float age;
	int strength;
};

/*--------------------------------------.
| Records statistics on the population. |
`--------------------------------------*/
struct PopulationStats
{
	int count_total;
	int count_diseased;
	int sum_strength;
	int sum_age;
};

/*--------------------------------------------------------------.
| Handles the population and draws updates to the image-buffer. |
`--------------------------------------------------------------*/
struct Map
{
	// Properties.
	const unsigned Width, Height;
	const unsigned TotalCells;

	// Grid display.
	std::vector<Person> population_grid;
	sf::Image image_buffer{};
	sf::Texture texture{};
	sf::RectangleShape surface{};

	// Constructor.
	Map(unsigned width, unsigned height) 
		: Width{ width }, 
			Height{ height }, 
			TotalCells{ Width*Height }, 
			population_grid { TotalCells, { false, false, sf::Color::White, false, false, 0.f, 0.f, 0 } } 
	{}

	// Accessing cells with `()`-operator.
	Person* operator()(unsigned x, unsigned y) { return &population_grid[y * Width + x]; } 
};

/*----------------------------------------------------.
| Specifies the numeric properties of the simulation. |
`----------------------------------------------------*/
struct Config
{
	const unsigned WindowWidth, WindowHeight;
	const unsigned MapWidth, MapHeight;
	const float DiseasedAgingFactor;
	const unsigned ChanceForDisease;
	const float MaxLengthDisease;
	const unsigned MinYearsUntilReproduce, MaxYearsUntilReproduce;
	const unsigned MinStartStrength, MaxStartStrength;
};

/*----------------------------------------------.
| Generate a random number from `min` to `max`. |
`----------------------------------------------*/
static std::mt19937 rand_engine;
static int generate_random(int min, int max)
{
	std::uniform_int_distribution<int> distribute{ min, max };
	return distribute(rand_engine);
}

/*-------------------------------------------------------.
| Randomly pick a field next to `start_x` and `start_y`. |
`-------------------------------------------------------*/
static sf::Vector2u random_destination(unsigned start_x, unsigned start_y, unsigned map_width, unsigned map_height)
{
	sf::Vector2u destination{ start_x, start_y };
	switch (generate_random(0, 4))
	{
	case 0: destination.x < map_width  ? ++destination.x : 0; break;
	case 1: destination.y < map_height ? ++destination.y : 0; break;
	case 2: destination.x > 0 ? --destination.x : 0;          break;
	case 3: destination.y > 0 ? --destination.y : 0;          break;
	}
	return destination;
}

/*------------------------------------------------------.
| Return the recorded statistics as a formatted string. |
`------------------------------------------------------*/
static std::string population_statistics_to_string(
	unsigned fps, 
	const std::map<sf::Uint32, PopulationStats>& population_stats, 
	const std::map<std::string, sf::Color>& global_colors
){
	// Get different teams.
	const auto& red_stats = population_stats.at(global_colors.at("team-red").toInteger());
	const auto& yellow_stats = population_stats.at(global_colors.at("team-yellow").toInteger());
	const auto& violet_stats = population_stats.at(global_colors.at("team-violet").toInteger());
	const auto& blue_stats = population_stats.at(global_colors.at("team-blue").toInteger());

	// Count the people that are alive.
	unsigned red_alive_total = red_stats.count_total;
	unsigned yellow_alive_total = yellow_stats.count_total;
	unsigned violet_alive_total = violet_stats.count_total;
	unsigned blue_alive_total = blue_stats.count_total;

	// Count the people that are diseased.
	unsigned red_diseased_total = red_stats.count_diseased;
	unsigned yellow_diseased_total = yellow_stats.count_diseased;
	unsigned violet_diseased_total = violet_stats.count_diseased;
	unsigned blue_diseased_total = blue_stats.count_diseased;

	// Calculate average strength.
	unsigned red_avg_str = red_stats.sum_strength / (red_alive_total > 0 ? red_alive_total : 1);
	unsigned yellow_avg_str = yellow_stats.sum_strength / (yellow_alive_total > 0 ? yellow_alive_total : 1);
	unsigned violet_avg_str = violet_stats.sum_strength / (violet_alive_total > 0 ? violet_alive_total : 1);
	unsigned blue_avg_str = blue_stats.sum_strength / (blue_alive_total > 0 ? blue_alive_total : 1);

	// Calculate average age.
	unsigned red_avg_age = red_stats.sum_age / (red_alive_total > 0 ? red_alive_total : 1);
	unsigned yellow_avg_age = yellow_stats.sum_age / (yellow_alive_total > 0 ? yellow_alive_total : 1);
	unsigned violet_avg_age = violet_stats.sum_age / (violet_alive_total > 0 ? violet_alive_total : 1);
	unsigned blue_avg_age = blue_stats.sum_age / (blue_alive_total > 0 ? blue_alive_total : 1);

	// Update fps-widget.
	return std::string{
		"PixelCiv v0.8 ~ Fps " + std::to_string(fps) + "\n" +
		"Red:    Alive(" + std::to_string(red_alive_total) +
		") Sick(" + std::to_string(red_diseased_total) +
		") AvgAge(" + std::to_string(red_avg_age) +
		") AvgStr(" + std::to_string(red_avg_str) + ")\n" +
		"Yellow: Alive(" + std::to_string(yellow_alive_total) +
		") Sick(" + std::to_string(yellow_diseased_total) +
		") AvgAge(" + std::to_string(yellow_avg_age) +
		") AvgStr(" + std::to_string(yellow_avg_str) + ")\n" +
		"Violet:  Alive(" + std::to_string(violet_alive_total) +
		") Sick(" + std::to_string(violet_diseased_total) +
		") AvgAge(" + std::to_string(violet_avg_age) +
		") AvgStr(" + std::to_string(violet_avg_str) + ")\n" +
		"Blue:   Alive(" + std::to_string(blue_alive_total) +
		") Sick(" + std::to_string(blue_diseased_total) +
		") AvgAge(" + std::to_string(blue_avg_age) +
		") AvgStr(" + std::to_string(blue_avg_str) + ")\n"
	};
}

/*------.
| Main. |
`------*/
int main()
{
	// Load config.
	Config config{ 
		1280, 720, // Window size. 
		640, 360,  // Map size.
		16.f,      // Increase aging by this factor for diseased people.
		20000,     // Chance of getting a disease (1 in x).
		2,         // The maximum number of years a disease can spread.
		3, 12,     // The minimum and maximum amount of time it takes a person to reproduce. 
		40, 85     // The smallest and largest possible strength value on startup.
	};


	// Load font.
	sf::Font ui_font;
	ui_font.loadFromFile("_font/Consolas.ttf");
	
	// FPS counter.
	sf::RectangleShape fps_widget_background{ sf::Vector2f{ 540.f, 120.f } };
	fps_widget_background.setPosition(0.f, 600.f);
	fps_widget_background.setFillColor(sf::Color{ 0,255,255, 140 });
	fps_widget_background.setOutlineThickness(2.f);
	fps_widget_background.setOutlineColor(sf::Color::Black);
	sf::Text fps_widget{ "", ui_font, 16 };
	fps_widget.setFillColor(sf::Color::Black);
	fps_widget.setPosition(10.f, 610.f);
	unsigned tick_counter = 0;
	float fps_time = 0.f;
	
	// Background map.
	sf::Texture background_map_texture;
	background_map_texture.loadFromFile("_texture/world_maps_seapath.png");
	const sf::Image BACKGROUND_MAP_IMAGE = background_map_texture.copyToImage();
	
	// Create map.
	Map map{ config.MapWidth, config.MapHeight };
	map.image_buffer = BACKGROUND_MAP_IMAGE;
	map.texture.loadFromImage(map.image_buffer);
	map.surface.setTexture(&(map.texture));
	map.surface.setSize(sf::Vector2f{ float(config.MapWidth), float(config.MapHeight) });
	
	// Create random tribes to test.
	auto create_tribe = [&](sf::Vector2i ul, sf::Vector2i lr, sf::Color color, unsigned total_population) {
		for (unsigned i = 0; i < total_population; ++i)
		{
			sf::Vector2i spawn_at_pos{ generate_random(ul.x, lr.x), generate_random(ul.y, lr.y) };
			if (map.image_buffer.getPixel(spawn_at_pos.x, spawn_at_pos.y) == sf::Color::Green)
			{
				bool rand_sex = (bool)generate_random(0, 2);
				float rand_reproduction = (float)generate_random(1, 20);
				float rand_age = (float)generate_random(1, 35);
				int rand_strength = generate_random(config.MinStartStrength, config.MaxStartStrength);
				*map(spawn_at_pos.x, spawn_at_pos.y) = { true, true, color, rand_sex, 0.f, rand_reproduction, rand_age, rand_strength };
				map.image_buffer.setPixel(spawn_at_pos.x, spawn_at_pos.y, color);
			}
		}
	};

	// Define colors.
	const std::map<std::string, sf::Color> global_colors{
		std::make_pair("tile-grass", sf::Color{    0, 255,   0 }),
		std::make_pair("tile-water", sf::Color{    0,   0, 255 }),
		std::make_pair("team-red",    sf::Color{ 255,   0,   0 }),
		std::make_pair("team-yellow", sf::Color{ 255, 200,   0 }),
		std::make_pair("team-violet", sf::Color{ 128,   0, 255 }),
		std::make_pair("team-blue",   sf::Color{   0, 128, 255 })
	};

	// Define starting positions for each team.
	create_tribe(sf::Vector2i{ 380, 60 }, sf::Vector2i{ 400, 80 }, global_colors.at("team-red"), 50);
	create_tribe(sf::Vector2i{ 400, 110 }, sf::Vector2i{ 420, 130 }, global_colors.at("team-blue"), 50);
	//create_tribe(sf::Vector2i{  50,  20 }, sf::Vector2i{ 500,  95 }, global_colors.at("team-red"),    500000);
	//create_tribe(sf::Vector2i{  50,  95 }, sf::Vector2i{ 500, 150 }, global_colors.at("team-yellow"), 500000);
	//create_tribe(sf::Vector2i{ 100, 150 }, sf::Vector2i{ 500, 220 }, global_colors.at("team-violet"), 500000);
	//create_tribe(sf::Vector2i{ 100, 220 }, sf::Vector2i{ 500, 310 }, global_colors.at("team-blue"),   500000);
	
	// Create window.
	sf::RenderWindow window{ sf::VideoMode{ config.WindowWidth, config.WindowHeight, 32 }, "PixelCiv 0.8", sf::Style::Default };
	window.setFramerateLimit(60);
	sf::Event main_event;
	sf::Clock frame_clock;
	sf::View map_view{ sf::Vector2f{ float(config.MapWidth / 2), float(config.MapHeight / 2) }, sf::Vector2f{ float(config.MapWidth), float(config.MapHeight) } };
	
	// Update timer.
	const float UPDATE_TIMER_MAX = 0.01f;
	float update_timer = 0.f;
	
	while (window.isOpen())
	{
		// Events.
		while (window.pollEvent(main_event))
		{
			if (main_event.type == sf::Event::KeyPressed)
			{
				if (main_event.key.code == sf::Keyboard::Escape)
					window.close();
			}
			if (main_event.type == sf::Event::Closed)
				window.close();
		}
	
		// Update timer.
		const float DELTA = frame_clock.restart().asSeconds();
		update_timer += DELTA;
		fps_time += DELTA;
		++tick_counter;

		// Record statistics on the population of each team.
		std::map<sf::Uint32, PopulationStats> population_stats{
			std::make_pair(global_colors.at("team-red").toInteger(),    PopulationStats{ 0, 0, 0, 0 }),
			std::make_pair(global_colors.at("team-yellow").toInteger(), PopulationStats{ 0, 0, 0, 0 }),
			std::make_pair(global_colors.at("team-violet").toInteger(), PopulationStats{ 0, 0, 0, 0 }),
			std::make_pair(global_colors.at("team-blue").toInteger(),   PopulationStats{ 0, 0, 0, 0 })
		};

		// Update on timer reaching max.
		if (update_timer >= UPDATE_TIMER_MAX)
		{
			update_timer = 0.f;
			map.image_buffer = BACKGROUND_MAP_IMAGE;
	
			// Lambda for updating population. Gets executed in multiple threads.
			auto update_population_in_range = [&](unsigned from_idx, unsigned length) {

				// Calculate position on x and y.
				unsigned idx_x = from_idx % map.Width;
				unsigned idx_y = from_idx / map.Width;

				// Get iterators to the beginning and end of the range.
				auto from = map.population_grid.begin() + from_idx;
				auto to = map.population_grid.begin() + from_idx + length;

				// Update each person in range.
				std::for_each(from, to, [&](Person& p) {

					if (p.active && p.is_updated)
					{
						// Dont update twice.
						p.is_updated = false;
						map.image_buffer.setPixel(idx_x, idx_y, p.color);
					}
					else if (p.active)
					{
						// Record stats.
						population_stats[p.color.toInteger()].count_total++;
						population_stats[p.color.toInteger()].sum_strength += p.strength;
						population_stats[p.color.toInteger()].sum_age += static_cast<int>(p.age);
						if (p.disease > 0.f) population_stats[p.color.toInteger()].count_diseased++;

						// Increase age and check if the person is dead.
						p.age += DELTA;
						if (p.age >= p.strength || p.age >= 85.f)
						{
							p = { false, false, sf::Color::White, 0.f, 0 };
						}

						// Decrease reproduction counter.
						if (!(p.is_male)/* && p.age > 18 && p.age < 60*/)
						{
							p.reproduction -= DELTA;
						}

						// Handle diseases.
						if (p.disease > 0.f)
						{
							p.age += (DELTA*config.DiseasedAgingFactor); // Increase the speed of aging when sick.
							p.disease -= DELTA;  // Decrease the remaining time of the disease.
						}
						else if(generate_random(0, config.ChanceForDisease) == 1) 
						{
							// Caught a disease.
							p.disease = (float)generate_random(1, int(config.MaxLengthDisease));
						}

						// Set different color if diseased.
						sf::Color pixel_color = (p.disease > 0.f ? sf::Color{ p.color.r, p.color.g, p.color.b, 160 } : p.color);

						// Calculate random neighbouring destination.
						sf::Vector2u destination{ random_destination(idx_x, idx_y, map.Width, map.Height) };

						// Get the field color of the destination.
						if (map.image_buffer.getPixel(destination.x, destination.y) == global_colors.at("tile-grass"))
						{
							Person* target = map(destination.x, destination.y);
							if (!(target->active))
							{
								if (!(p.is_male) && p.reproduction <= 0.f)
								{
									// Reset reproduction rate.
									p.reproduction = (float)generate_random(config.MinYearsUntilReproduce, config.MaxYearsUntilReproduce);

									// Create baby at destination.
									*target = p;
									target->is_male = (bool)generate_random(0, 2);
									target->reproduction = (float)generate_random(config.MinYearsUntilReproduce, config.MaxYearsUntilReproduce);
									target->strength = generate_random((p.strength > 15 ? p.strength - 15 : 15), p.strength + 30);
									target->age = 1.f;
									target->is_updated = true;
									map.image_buffer.setPixel(destination.x, destination.y, pixel_color);
								}
								else
								{
									// Walk to the destination if its not blocked by another person.
									*target = p;
									p.active = false;
									target->is_updated = true;
									map.image_buffer.setPixel(destination.x, destination.y, pixel_color);
								}
							}
							else if (target->color == p.color)
							{
								// Infect someone with a disease.
								if (p.disease > 0.f && generate_random(0, 2) == 1)
								{
									target->disease = p.disease;
								}
								map.image_buffer.setPixel(idx_x, idx_y, pixel_color);
							}
							else if (target->color != p.color)
							{
								// Fight an enemy.
								if (target->strength > p.strength)
									p.age = static_cast<float>(p.strength);
								else
									target->age = static_cast<float>(p.strength);
								map.image_buffer.setPixel(idx_x, idx_y, pixel_color);
							}
							else
							{
								map.image_buffer.setPixel(idx_x, idx_y, pixel_color);
							}
						}
						else
						{
							map.image_buffer.setPixel(idx_x, idx_y, pixel_color);
						}
					}

					// Update index counter.
					if (++idx_x >= map.Width)
					{
						idx_x = 0;
						++idx_y;
					}
				});
			};

			// Start threads that update the population.
			const std::array<sf::Thread*, 4> thread_list = {
				new sf::Thread{ std::bind(update_population_in_range, 0, map.TotalCells / 4) },
				new sf::Thread{ std::bind(update_population_in_range, (map.TotalCells / 4), map.TotalCells / 4) },
				new sf::Thread{ std::bind(update_population_in_range, (map.TotalCells / 4) * 2, map.TotalCells / 4) },
				new sf::Thread{ std::bind(update_population_in_range, (map.TotalCells / 4) * 3, map.TotalCells / 4) }
			};

			// Launch threads and wait for completion.
			std::for_each(thread_list.begin(), thread_list.end(), [](sf::Thread* th) { th->launch(); });
			std::for_each(thread_list.begin(), thread_list.end(), [](sf::Thread* th) { th->wait(); delete th; }); 

			// Apply image_buffer to the render-texture.
			map.texture.loadFromImage(map.image_buffer);
	
			// Draw to screen.
			window.clear();
			window.setView(map_view);
			window.draw(map.surface);
			window.setView(window.getDefaultView());
			window.draw(fps_widget_background);
			window.draw(fps_widget);
			window.display();
		}
	
		// Late update.
		if (fps_time >= 1.f)
		{
			// Update fps-widget.
			fps_widget.setString(population_statistics_to_string(tick_counter, population_stats, global_colors));
			fps_time = 0.f;
			tick_counter = 0;
		}
	}
	return 0;
}
