#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <thread>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "core/data_buffer.h"
#include "core/exception.h"
#include "core/looper.h"
#include "core/root.h"
#include "core/vector3.h"
#include "log/log.h"
#include "networking/data_buffer_deserialiser.h"
#include "networking/data_buffer_serialiser.h"
#include "networking/packet.h"
#include "networking/posix/auto_socket.h"
#include "networking/server_connection_handler.h"
#include "networking/simulated_accepting_socket.h"
#include "networking/udp_accepting_socket.h"
#include "physics/basic_character_controller.h"
#include "physics/box_rigid_body.h"
#include "physics/physics_system.h"
#include "platform/keyboard_event.h"

#include "client_input.h"

using namespace std::chrono_literals;

eng::CharacterController *character_controller = nullptr;
std::size_t player_id = std::numeric_limits<std::size_t>::max();

void go(int, char **)
{
    eng::Root::logger().set_log_engine(true);

    LOG_DEBUG("server_sample", "hello world");

    std::deque<ClientInput> inputs;
    auto tick = 0u;

    auto socket = std::make_unique<eng::SimulatedAcceptingSocket>(
        "2",
        "1",
        0ms,
        0ms,
        0.0f);

    eng::ServerConnectionHandler connection_handler(
        std::move(socket),
        [](std::size_t id)
        {
            LOG_DEBUG("server", "new connection {}", id);

            // just support a single player
            player_id = id;
        },
        [&inputs, &tick](std::size_t id, const eng::DataBuffer &data, eng::ChannelType type)
        {
            if(type == eng::ChannelType::RELIABLE_ORDERED)
            {
                eng::DataBufferDeserialiser deserialiser(data);
                ClientInput input{ deserialiser };

                if(input.tick >= tick)
                {
                    // if input is for now or the future (which it should be as
                    // the client runs ahead) then store it
                    inputs.emplace_back(input);
                }
                else
                {
                    LOG_WARN("server", "stale input: {} {}", tick, input.tick);
                }
            }
    });

    eng::PhysicsSystem ps{ };
    character_controller = ps.create_character_controller<eng::BasicCharacterController>();
    ps.create_rigid_body<eng::BoxRigidBody>(eng::Vector3{ 0.0f, -50.0f, 0.0f }, eng::Vector3{ 500.0f, 50.0f, 500.0f }, true);
    auto *box = ps.create_rigid_body<eng::BoxRigidBody>(eng::Vector3{ 0.0f, 1.0f, 0.0f }, eng::Vector3{ 0.5f, 0.5f, 0.5f }, false);

    // block and wait for client to connect
    while(player_id == std::numeric_limits<std::size_t>::max())
    {
        connection_handler.update();
        std::this_thread::sleep_for(100ms);
    }

    std::chrono::microseconds step(0);

    ps.step(33ms);

    eng::Looper looper{
        0ms,
        33ms,
        [&](std::chrono::microseconds clock, std::chrono::microseconds time_step)
        {
            // fixed timestep function
            // this runs the physics and processes player input

            for(const auto &input : inputs)
            {
                // if stored input is for our current tick then apply it to
                // the physics simulation
                if(input.tick == tick)
                {
                    eng::Vector3 walk_direction{ input.side, 0.0f, input.forward };
                    walk_direction.normalise();

                    character_controller->set_walk_direction(walk_direction);
                }
                if(input.tick > tick)
                {
                    break;
                }
            }

            ps.step(33ms);
            ++tick;

            return true;
        },
        [&](std::chrono::microseconds clock, std::chrono::microseconds time_step)
        {
            // variable timestep function
            // sends snapshots of the world to the client

            connection_handler.update();

            // whilst this is a variable time function we only want to send out
            // updates every 100ms
            if(clock > step + 100ms)
            {
                // serialise world state
                eng::DataBufferSerialiser serialiser;
                serialiser.push(character_controller->position());
                serialiser.push(character_controller->linear_velocity());
                serialiser.push(character_controller->angular_velocity());
                serialiser.push(tick);
                serialiser.push(box->position());
                serialiser.push(box->orientation());

                connection_handler.send(player_id, serialiser.data(), eng::ChannelType::RELIABLE_ORDERED);

                step = clock;
            }

            return true;
        }};

    looper.run();
}

int main(int argc, char **argv)
{
    try
    {
        go(argc, argv);
    }
    catch(eng::Exception &e)
    {
        LOG_ERROR("server", e.what());
        LOG_ERROR("server", e.stack_trace());
    }
    catch(...)
    {
        LOG_ERROR("server", "unknown exception");
    }

    return 0;
}



