////////////////////////////////////////////////////////////////////////////////
//         Distributed under the Boost Software License, Version 1.0.         //
//            (See accompanying file LICENSE or copy at                       //
//                 https://www.boost.org/LICENSE_1_0.txt)                     //
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace iris
{

class JobSystemManager;
class MaterialManager;
class RenderTargetManager;
class MeshManager;
class PhysicsManager;
class TextureManager;
class WindowManager;

/**
 * This class allows for the runtime registration and retrieval of various
 * manager classes. It is a singleton and therefore provides singleton access to
 * the various components in owns without requiring them to be singletons.
 *
 * These managers are factory classes that can create engine components, the
 * reason for all this machinery is:
 *  - it decouples actual implementation from the Root
 *  - start() can register all supported mangers for the current platform and
 *    set sane defaults
 *  - allows a user to register their own implementations (e.g. for a different
 *    physics library)
 *
 * Note that there is a subtle difference between setting the graphics/physics
 * apis and the jobs api. Graphics/Physics are entirely a user choice, they may
 * want one, both or neither. The Root makes this possible as they only need to
 * get the manager for the components they need and call the varies create
 * methods.
 *
 * Whereas physics/graphics are opt-in the jobs api is compulsory. The engine
 * has to have a jobs system. Therefore setting the jobs api (set_jobs_api())
 * will actually create the job system. To make things a bit less verbose the
 * JobsApiManager interface exposes the two job system api methods, so a user
 * can use jobs directly from the jobs_manager() call.
 */
class Root
{
  public:
    ~Root() = default;

    /**
     * Get the current WindowManager.
     *
     * @returns
     *   Current WindowManager.
     */
    static WindowManager &window_manager();

    /**
     * Get the current MeshManager.
     *
     * @returns
     *   Current MeshManager.
     */
    static MeshManager &mesh_manager();

    /**
     * Get the current TextureManager.
     *
     * @returns
     *   Current TextureManager.
     */
    static TextureManager &texture_manager();

    /**
     * Get the current MaterialManager.
     *
     * @returns
     *   Current MaterialManager.
     */
    static MaterialManager &material_manager();

    /**
     * Get the current RenderTargetManager.
     *
     * @returns
     *   Current RenderTargetManager.
     */
    static RenderTargetManager &render_target_manager();

    /**
     * Get the current PhysicsManager.
     *
     * @returns
     *   Current PhysicsManager.
     */
    static PhysicsManager &physics_manager();

    /**
     * Get the current JobSystemManager.
     *
     * @returns
     *   Current JobSystemManager.
     */
    static JobSystemManager &jobs_manager();

    /**
     * Register managers for a given api name.
     *
     * @param api
     *   Name of api to register managers to,
     *
     * @param window_manager
     *   New WindowManager.
     *
     * @param mesh_manager
     *   New MeshManager.
     *
     * @param texture_manager
     *   New TextureManager.
     *
     * @param material_manager
     *   New MaterialManager.
     *
     * @param RenderTargetManager
     *   New RenderTargetManager.
     */
    static void register_graphics_api(
        const std::string &api,
        std::unique_ptr<WindowManager> window_manager,
        std::unique_ptr<MeshManager> mesh_manager,
        std::unique_ptr<TextureManager> texture_manager,
        std::unique_ptr<MaterialManager> material_manager,
        std::unique_ptr<RenderTargetManager> render_target_manager);

    /**
     * Get the currently set graphics api.
     *
     * @returns
     *   Name of currently set graphics api.
     */
    static std::string graphics_api();

    /**
     * Set the current graphics api.
     *
     * @param api
     *   New graphics api name.
     */
    static void set_graphics_api(const std::string &api);

    /**
     * Get a collection of all registered api names.
     *
     * @returns
     *   Collection of registered api names.
     */
    static std::vector<std::string> registered_graphics_apis();

    /**
     * Register managers for a given api name.
     *
     * @param api
     *   Name of api to register managers to,
     *
     * @param physics_manager
     *   New PhysicsManager.
     */
    static void register_physics_api(const std::string &api, std::unique_ptr<PhysicsManager> physics_manager);

    /**
     * Get the currently set physics api.
     *
     * @returns
     *   Name of currently set physics api.
     */
    static std::string physics_api();

    /**
     * Set the current physics api.
     *
     * @param api
     *   New physics api name.
     */
    static void set_physics_api(const std::string &api);

    /**
     * Get a collection of all registered api names.
     *
     * @returns
     *   Collection of registered api names.
     */
    static std::vector<std::string> registered_physics_apis();

    /**
     * Register managers for a given api name.
     *
     * @param api
     *   Name of api to register managers to,
     *
     * @param jobs_manager
     *   New JobSystemManager.
     */
    static void register_jobs_api(const std::string &api, std::unique_ptr<JobSystemManager> jobs_manager);

    /**
     * Get the currently set jobs api.
     *
     * @returns
     *   Name of currently set jobs api.
     */
    static std::string jobs_api();

    /**
     * Set the current jobs api.
     *
     * @param api
     *   New jobs api name.
     */
    static void set_jobs_api(const std::string &api);

    /**
     * Get a collection of all registered api names.
     *
     * @returns
     *   Collection of registered api names.
     */
    static std::vector<std::string> registered_jobs_apis();

    /**
     * Clear all registered components.
     *
     * This method exists to allow the engine to destroy the internal managers
     * at a time of its choosing, rather than waiting for the singleton itself
     * to be destroyed. There is no reason a user should have to call this.
     */
    static void reset();

  private:
    // private to force access through above public static methods
    Root();
    static Root &instance();

    // these are the member function equivalents of the above static methods
    // see their docs for details

    WindowManager &window_manager_impl() const;

    MeshManager &mesh_manager_impl() const;

    TextureManager &texture_manager_impl() const;

    MaterialManager &material_manager_impl() const;

    RenderTargetManager &render_target_manager_impl() const;

    PhysicsManager &physics_manager_impl() const;

    JobSystemManager &jobs_manager_impl() const;

    void register_graphics_api_impl(
        const std::string &api,
        std::unique_ptr<WindowManager> window_manager,
        std::unique_ptr<MeshManager> mesh_manager,
        std::unique_ptr<TextureManager> texture_manager,
        std::unique_ptr<MaterialManager> material_manager,
        std::unique_ptr<RenderTargetManager> render_target_manager);

    std::string graphics_api_impl() const;

    void set_graphics_api_impl(const std::string &api);

    std::vector<std::string> registered_graphics_apis_impl() const;

    void register_physics_api_impl(const std::string &api, std::unique_ptr<PhysicsManager> physics_manager);

    std::string physics_api_impl() const;

    void set_physics_api_impl(const std::string &api);

    std::vector<std::string> registered_physics_apis_impl() const;

    void register_jobs_api_impl(const std::string &api, std::unique_ptr<JobSystemManager> jobs_manager);

    std::string jobs_api_impl() const;

    void set_jobs_api_impl(const std::string &api);

    std::vector<std::string> registered_jobs_apis_impl() const;

    void reset_impl();

    /**
     * Helper struct encapsulating all managers for a graphics api.
     *
     * Note that the member order is important, we want the WindowManager to
     * be destroyed first as some implementations require the Renderer
     * destructor to wait for gpu operations to finish before destroying other
     * resources.
     */
    struct GraphicsApiManagers
    {
        std::unique_ptr<MeshManager> mesh_manager;
        std::unique_ptr<TextureManager> texture_manager;
        std::unique_ptr<MaterialManager> material_manager;
        std::unique_ptr<RenderTargetManager> render_target_manager;
        std::unique_ptr<WindowManager> window_manager;
    };

    /** Map of graphics api name to managers. */
    std::unordered_map<std::string, GraphicsApiManagers> graphics_api_managers_;

    /** Name of current graphics api. */
    std::string graphics_api_;

    /** Map of physics api name to managers. */
    std::unordered_map<std::string, std::unique_ptr<PhysicsManager>> physics_api_managers_;

    /** Name of current physics api. */
    std::string physics_api_;

    /** Map of jobs api name to managers. */
    std::unordered_map<std::string, std::unique_ptr<JobSystemManager>> jobs_api_managers_;

    /** Name of current jobs api. */
    std::string jobs_api_;
};

}
