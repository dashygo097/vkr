#include "vkr/ui/ui.hh"
#include "vkr/core/core_utils.hh"
#include "vkr/logger.hh"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <imgui_internal.h>
#include <algorithm>
#include <filesystem>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

namespace vkr::ui {

UI::UI(const core::Window &window, const core::Instance &instance,
       const core::Surface &surface, const core::Device &device,
       const core::CommandPool &commandPool,
       resource::ResourceManager &resourceManager,
       const util::AssetSystem &assetSystem,
       scene::CameraDesc &camera,
       resource::OffscreenTarget &offscreenTarget,
       const pipeline::RenderPass &renderPass,
       const pipeline::DescriptorPool &descriptorPool,
       render::RenderGraph &renderGraph, util::Timer &timer, ThemeDesc &desc)
    : window_(window), instance_(instance), surface_(surface), device_(device),
      command_pool_(commandPool), resource_manager_(resourceManager),
      asset_system_(assetSystem), camera_(camera),
      offscreen_target_(offscreenTarget), render_pass_(renderPass),
      descriptor_pool_(descriptorPool), render_graph_(renderGraph),
      timer_(timer), desc_(desc) {
  dock_panels_ = {
      DockPanelState{DockPanelId::Viewport, "Viewport", true, true},
      DockPanelState{DockPanelId::Resources, "Resources", true, true},
      DockPanelState{DockPanelId::RenderGraph, "Render Graph", true, true},
      DockPanelState{DockPanelId::Assets, "Assets", true, true},
      DockPanelState{DockPanelId::Camera, "Camera", true, true},
      DockPanelState{DockPanelId::MeshEditor, "Mesh Editor", true, true},
      DockPanelState{DockPanelId::ShaderEditor, "Shader Editor", true, true},
      DockPanelState{DockPanelId::Performance, "Performance", true, true},
      DockPanelState{DockPanelId::Logging, "Logging", true, true},
  };

  VKR_UI_INFO("Initializing ImGui UI...");
  IMGUI_CHECKVERSION();

  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  Theme::apply(desc_);

  ImGuiStyle &style = ImGui::GetStyle();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  ImGui_ImplGlfw_InitForVulkan(window_.glfwWindow(), true);

  ImGui_ImplVulkan_PipelineInfo pipelineInfo{};
  pipelineInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  pipelineInfo.RenderPass = render_pass_.renderPass();

  ImGui_ImplVulkan_InitInfo initInfo{};
  initInfo.Instance = instance_.instance();
  initInfo.PhysicalDevice = device_.physicalDevice();
  initInfo.Device = device_.device();
  initInfo.QueueFamily = device_.graphicsFamily();
  initInfo.Queue = device_.graphicsQueue();
  initInfo.PipelineCache = VK_NULL_HANDLE;
  initInfo.DescriptorPool = descriptor_pool_.pool();
  initInfo.Allocator = nullptr;
  initInfo.MinImageCount = core::MAX_FRAMES_IN_FLIGHT;
  initInfo.ImageCount = core::MAX_FRAMES_IN_FLIGHT;
  initInfo.CheckVkResultFn = core::check_vk_result;
  initInfo.PipelineInfoMain = pipelineInfo;

  ImGui_ImplVulkan_Init(&initInfo);

  std::vector<pipeline::DescriptorBinding> offscreenBindings = {
      {.name = "offscreen",
       .layout = {
           .binding = 0,
           .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
           .descriptorCount = 1,
           .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
       }}};

  offscreen_descriptor_layout_ =
      std::make_unique<pipeline::DescriptorSetLayout>(device_);
  offscreen_descriptor_layout_->update({.bindings = offscreenBindings});

  VkDescriptorImageInfo offscreenImageInfo{};
  offscreenImageInfo.sampler = offscreen_target_.color().sampler();
  offscreenImageInfo.imageView = offscreen_target_.color().imageView();
  offscreenImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  auto offscreenWrite = pipeline::DescriptorSetWriteDesc::forSet(0);
  offscreenWrite.images.push_back(pipeline::DescriptorImageWriteDesc::one(
      0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, offscreenImageInfo));

  offscreen_descriptor_sets_ =
      std::make_unique<pipeline::DescriptorSets>(device_);
  offscreen_descriptor_sets_->update(pipeline::DescriptorSetsDesc{
      .pool = descriptor_pool_.pool(),
      .layout = offscreen_descriptor_layout_->layout(),
      .setCount = 1,
      .writes = {offscreenWrite},
  });

  VKR_UI_INFO("Initializing FPS Panel...");
  fps_panel_ = std::make_unique<FPSPanel>(timer);
  fps_panel_->clear();
  VKR_UI_INFO("FPS Panel initialized successfully.");

  VKR_UI_INFO("Initializing Shader Editor...");
  shader_editor_ = std::make_unique<ShaderEditor>(render_graph_);
  VKR_UI_INFO("Shader Editor initialized successfully.");

  VKR_UI_INFO("Initializing Logging Panel...");
  logging_panel_ = std::make_unique<LoggingPanel>();
  VKR_UI_INFO("Logging Panel initialized successfully.");

  VKR_UI_INFO("Initializing Resource Tree...");
  resource_tree_ = std::make_unique<ResourceTree>(resource_manager_);
  VKR_UI_INFO("Resource Tree initialized successfully.");

  VKR_UI_INFO("ImGui UI initialized successfully.");
}

UI::~UI() {
  shader_editor_.reset();
  logging_panel_.reset();
  fps_panel_.reset();
  resource_tree_.reset();

  offscreen_descriptor_sets_.reset();
  offscreen_descriptor_layout_.reset();

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void UI::render(VkCommandBuffer commandBuffer) {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  switch (layout_mode_) {
  case LayoutMode::FullScreen:
    renderMainMenu();
    renderFullScreen();
    break;
  case LayoutMode::Standard:
    renderDockspace();
    renderWorkspacePanels();
    break;
  }

  ImGui::Render();
  ImDrawData *drawData = ImGui::GetDrawData();
  ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
}

void UI::renderFullScreen() {
  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

  ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

  if (ImGui::Begin("Fullscreen Viewport", nullptr, flags)) {
    if (offscreen_descriptor_sets_ &&
        !offscreen_descriptor_sets_->sets().empty()) {
      ImGui::Image(
          reinterpret_cast<ImTextureID>(offscreen_descriptor_sets_->sets()[0]),
          ImGui::GetContentRegionAvail());
    }
  }

  ImGui::End();
  ImGui::PopStyleVar(2);
}

void UI::renderDockspace() {
  static bool dockspaceOpen = true;
  static ImGuiDockNodeFlags dockSpaceFlags = ImGuiDockNodeFlags_None;

  renderMainMenu();

  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;

  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

  windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
  windowFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  windowFlags |=
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  if (dockSpaceFlags & ImGuiDockNodeFlags_PassthruCentralNode) {
    windowFlags |= ImGuiWindowFlags_NoBackground;
  }

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("DockSpace", &dockspaceOpen, windowFlags);
  ImGui::PopStyleVar();
  ImGui::PopStyleVar(2);

  ImGuiIO &io = ImGui::GetIO();
  if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
    ImGuiID dockSpaceId = ImGui::GetID("DockSpace");
    ImGui::DockSpace(dockSpaceId, ImVec2(0.0f, 0.0f), dockSpaceFlags);

    if (dock_layout_dirty_) {
      dock_layout_dirty_ = false;
      setupDockingLayout();
    }
  }

  ImGui::End();
}

void UI::setupDockingLayout() {
  ImGuiID dockSpaceId = ImGui::GetID("DockSpace");
  ImGui::DockBuilderRemoveNode(dockSpaceId);
  ImGui::DockBuilderAddNode(dockSpaceId, ImGuiDockNodeFlags_DockSpace);
  ImGui::DockBuilderSetNodeSize(dockSpaceId, ImGui::GetMainViewport()->Size);

  ImGuiID dockLeft;
  ImGuiID dockLeftBottom;
  ImGuiID dockRight;
  ImGuiID dockRightBottom;
  ImGuiID dockRightTop;
  ImGuiID dockBottom;

  dockLeft = ImGui::DockBuilderSplitNode(dockSpaceId, ImGuiDir_Left, 0.2f,
                                         nullptr, &dockSpaceId);

  dockRight = ImGui::DockBuilderSplitNode(dockSpaceId, ImGuiDir_Right, 0.2f,
                                          nullptr, &dockSpaceId);

  dockRightTop = ImGui::DockBuilderSplitNode(dockRight, ImGuiDir_Up, 0.7f,
                                             nullptr, &dockRightBottom);

  dockBottom = ImGui::DockBuilderSplitNode(dockSpaceId, ImGuiDir_Down, 0.22f,
                                           nullptr, &dockSpaceId);
  dockLeftBottom = ImGui::DockBuilderSplitNode(dockLeft, ImGuiDir_Down, 0.48f,
                                               nullptr, &dockLeft);

  ImGui::DockBuilderDockWindow("Resources", dockLeft);
  ImGui::DockBuilderDockWindow("Assets", dockLeft);
  ImGui::DockBuilderDockWindow("Render Graph", dockLeftBottom);
  ImGui::DockBuilderDockWindow("Camera", dockLeftBottom);
  ImGui::DockBuilderDockWindow("Mesh Editor", dockLeftBottom);
  ImGui::DockBuilderDockWindow("Shader Editor", dockRightTop);
  ImGui::DockBuilderDockWindow("Performance", dockRightBottom);
  ImGui::DockBuilderDockWindow("Logging", dockBottom);
  ImGui::DockBuilderDockWindow("Viewport", dockSpaceId);

  ImGui::DockBuilderFinish(dockSpaceId);
}

void UI::resetDockingLayout() noexcept {
  for (auto &dockPanel : dock_panels_) {
    dockPanel.open = dockPanel.defaultOpen;
  }

  dock_layout_dirty_ = true;
}

void UI::renderMainMenu() {
  if (!ImGui::BeginMainMenuBar()) {
    return;
  }

  if (ImGui::BeginMenu("File")) {
    if (ImGui::MenuItem("Exit")) {
      should_close_ = true;
    }

    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("View")) {
    if (ImGui::MenuItem("Fullscreen Viewport", nullptr,
                        layout_mode_ == LayoutMode::FullScreen)) {
      layout_mode_ = LayoutMode::FullScreen;
    }

    if (ImGui::MenuItem("Workspace", nullptr,
                        layout_mode_ == LayoutMode::Standard)) {
      layout_mode_ = LayoutMode::Standard;
    }

    if (ImGui::MenuItem("Reset Dock Layout")) {
      resetDockingLayout();
      layout_mode_ = LayoutMode::Standard;
    }

    ImGui::SeparatorText("Panels");

    for (auto &dockPanel : dock_panels_) {
      if (ImGui::MenuItem(dockPanel.name, nullptr, dockPanel.open)) {
        dockPanel.open = !dockPanel.open;
      }
    }

    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("Theme")) {
    renderThemeControls();
    ImGui::EndMenu();
  }

  ImGui::Separator();
  ImGui::TextDisabled("%.1f FPS", timer_.fps());

  ImGui::EndMainMenuBar();
}

void UI::renderWorkspacePanels() {
  if (panelOpen(DockPanelId::Viewport)) {
    renderMainViewport();
  }

  if (panelOpen(DockPanelId::Resources)) {
    renderResourcePanel();
  }

  if (panelOpen(DockPanelId::RenderGraph)) {
    renderGraphPanel();
  }

  if (panelOpen(DockPanelId::Assets)) {
    renderAssetsPanel();
  }

  if (panelOpen(DockPanelId::Camera)) {
    renderCameraPanel();
  }

  if (panelOpen(DockPanelId::MeshEditor)) {
    renderMeshEditorPanel();
  }

  if (panelOpen(DockPanelId::ShaderEditor)) {
    renderShaderEditor();
  }

  if (panelOpen(DockPanelId::Performance)) {
    renderPerformancePanel();
  }

  if (panelOpen(DockPanelId::Logging)) {
    renderLoggingPanel();
  }
}

void UI::renderMainViewport() {
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;
  auto &viewportPanel = panel(DockPanelId::Viewport);

  if (ImGui::Begin(viewportPanel.name, &viewportPanel.open, windowFlags)) {
    ImVec2 panelSize = ImGui::GetContentRegionAvail();

    if (panelSize.x < 1.0f) {
      panelSize.x = 1.0f;
    }

    if (panelSize.y < 1.0f) {
      panelSize.y = 1.0f;
    }

    if (offscreen_descriptor_sets_ &&
        !offscreen_descriptor_sets_->sets().empty()) {
      ImGui::Image(
          reinterpret_cast<ImTextureID>(offscreen_descriptor_sets_->sets()[0]),
          panelSize);
    } else {
      ImGui::TextDisabled("(no offscreen target)");
    }

    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
    ImVec2 contentMax = ImGui::GetWindowContentRegionMax();

    viewport_info_.x = windowPos.x + contentMin.x;
    viewport_info_.y = windowPos.y + contentMin.y;
    viewport_info_.width = contentMax.x - contentMin.x;
    viewport_info_.height = contentMax.y - contentMin.y;
    viewport_info_.isFocused =
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    viewport_info_.isHovered = ImGui::IsWindowHovered();
  }

  ImGui::End();
  ImGui::PopStyleVar();
}

void UI::renderGraphPanel() {
  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
  auto &graphPanel = panel(DockPanelId::RenderGraph);

  if (ImGui::Begin(graphPanel.name, &graphPanel.open, windowFlags)) {
    const auto passes = render_graph_.passes();

    ImGui::SeparatorText("Passes");
    ImGui::TextDisabled("%zu total", passes.size());
    ImGui::Spacing();

    for (const auto *pass : passes) {
      if (pass == nullptr) {
        continue;
      }

      ImGuiTreeNodeFlags flags =
          ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;

      const bool open = ImGui::TreeNodeEx(pass->name().c_str(), flags, "%s",
                                          pass->name().c_str());

      if (open) {
        const auto *pipeline = pass->editablePipeline();
        if (pipeline != nullptr) {
          ImGui::Text("Pipeline: %s", pipeline->desc().name.empty()
                                          ? "<unnamed>"
                                          : pipeline->desc().name.c_str());
        } else {
          ImGui::TextDisabled("Pipeline: none");
        }

        if (ImGui::TreeNodeEx("Reads", ImGuiTreeNodeFlags_SpanAvailWidth)) {
          if (pass->reads().empty()) {
            ImGui::TextDisabled("None");
          } else {
            for (const auto &resource : pass->reads()) {
              ImGui::BulletText("%s", resource.c_str());
            }
          }
          ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Writes", ImGuiTreeNodeFlags_SpanAvailWidth)) {
          if (pass->writes().empty()) {
            ImGui::TextDisabled("None");
          } else {
            for (const auto &resource : pass->writes()) {
              ImGui::BulletText("%s", resource.c_str());
            }
          }
          ImGui::TreePop();
        }

        ImGui::TreePop();
      }
    }
  }

  ImGui::End();
}

void UI::renderResourcePanel() {
  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
  auto &resourcesPanel = panel(DockPanelId::Resources);

  if (ImGui::Begin(resourcesPanel.name, &resourcesPanel.open, windowFlags)) {
    if (resource_tree_) {
      resource_tree_->render();
    }
  }

  ImGui::End();
}

void UI::renderAssetsPanel() {
  auto &assetsPanel = panel(DockPanelId::Assets);

  if (ImGui::Begin(assetsPanel.name, &assetsPanel.open)) {
    const auto normalizeRoot = [](const std::string &root) {
      auto path = std::filesystem::path(root);
      if (path.is_relative()) {
        path = std::filesystem::absolute(path);
      }

      std::error_code ec;
      auto canonical = std::filesystem::weakly_canonical(path, ec);
      return ec ? path.lexically_normal() : canonical.lexically_normal();
    };

    const auto engineRoot = normalizeRoot(asset_system_.desc().engineRoot);
    const auto appRoot = normalizeRoot(asset_system_.desc().appRoot);
    const auto userRoot = normalizeRoot(asset_system_.desc().userRoot);

    const auto isInsideRoot = [](const std::filesystem::path &path,
                                 const std::filesystem::path &root) {
      std::error_code ec;
      auto relative = std::filesystem::relative(path, root, ec);
      if (ec || relative.empty()) {
        return false;
      }

      for (const auto &part : relative) {
        if (part == "..") {
          return false;
        }
      }

      return true;
    };

    auto renderRoot = [&](const char *label, const std::filesystem::path &root,
                          bool hidden, bool defaultOpen) {
      ImGuiTreeNodeFlags flags =
          ImGuiTreeNodeFlags_SpanAvailWidth;

      if (defaultOpen) {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
      }

      if (!ImGui::TreeNodeEx(label, flags, "%s", label)) {
        return;
      }

      ImGui::TextDisabled("%s", root.string().c_str());

      if (hidden) {
        ImGui::TextDisabled("Hidden because this root is engine defaults.");
        ImGui::TreePop();
        return;
      }

      if (!std::filesystem::exists(root)) {
        ImGui::TextDisabled("Not found");
        ImGui::TreePop();
        return;
      }

      std::vector<std::filesystem::path> entries{};
      std::error_code ec;
      std::filesystem::recursive_directory_iterator iterator(
          root, std::filesystem::directory_options::skip_permission_denied, ec);
      std::filesystem::recursive_directory_iterator end{};

      size_t skippedEngineDefaults = 0;
      size_t skippedDirectories = 0;
      size_t visibleLimit = 300;
      bool truncated = false;

      for (; iterator != end && !ec; iterator.increment(ec)) {
        if (ec) {
          break;
        }

        const auto path = iterator->path();
        const auto filename = path.filename().string();

        if (iterator->is_directory(ec)) {
          if (filename == ".git" || filename == "build" ||
              filename == "3rdparty" || filename == ".cache") {
            iterator.disable_recursion_pending();
            skippedDirectories++;
          }
          continue;
        }

        if (isInsideRoot(path, engineRoot)) {
          skippedEngineDefaults++;
          continue;
        }

        if (!iterator->is_regular_file(ec)) {
          continue;
        }

        if (entries.size() >= visibleLimit) {
          truncated = true;
          continue;
        }

        entries.push_back(path);
      }

      std::sort(entries.begin(), entries.end());

      if (entries.empty()) {
        ImGui::TextDisabled("No files");
      } else {
        for (const auto &path : entries) {
          std::error_code relEc;
          auto rel = std::filesystem::relative(path, root, relEc);
          ImGui::BulletText("%s",
                            (relEc ? path.filename() : rel).string().c_str());
        }
      }

      if (truncated) {
        ImGui::TextDisabled("Showing first %zu files", visibleLimit);
      }

      if (skippedEngineDefaults > 0 || skippedDirectories > 0) {
        ImGui::TextDisabled("Hidden: %zu engine files, %zu large folders",
                            skippedEngineDefaults, skippedDirectories);
      }

      ImGui::TreePop();
    };

    ImGui::SeparatorText("Target Roots");
    ImGui::Checkbox("App", &show_app_assets_);
    ImGui::SameLine();
    ImGui::Checkbox("User", &show_user_assets_);

    ImGui::SeparatorText("Project Assets");
    if (show_app_assets_) {
      renderRoot("App", appRoot, appRoot == engineRoot, true);
    }

    if (show_user_assets_) {
      renderRoot("User", userRoot, userRoot == engineRoot, false);
    }

    if (!show_app_assets_ && !show_user_assets_) {
      ImGui::TextDisabled("No asset root selected");
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Engine Defaults");
    ImGui::TextDisabled("Engine default assets are hidden from this browser.");
  }

  ImGui::End();
}

void UI::renderCameraPanel() {
  auto &cameraPanel = panel(DockPanelId::Camera);

  if (ImGui::Begin(cameraPanel.name, &cameraPanel.open)) {
    bool vectorsChanged = false;

    ImGui::SeparatorText("Transform");
    ImGui::DragFloat3("Position", glm::value_ptr(camera_.pos), 0.05f);

    vectorsChanged |=
        ImGui::SliderFloat("Yaw", &camera_.yaw, -180.0f, 180.0f, "%.1f deg");
    vectorsChanged |=
        ImGui::SliderFloat("Pitch", &camera_.pitch, -89.0f, 89.0f, "%.1f deg");

    if (vectorsChanged) {
      refreshCameraVectors();
    }

    ImGui::Text("Front: %.2f, %.2f, %.2f", camera_.front.x, camera_.front.y,
                camera_.front.z);
    ImGui::Text("Up: %.2f, %.2f, %.2f", camera_.up.x, camera_.up.y,
                camera_.up.z);

    ImGui::SeparatorText("Lens");
    ImGui::SliderFloat("FOV", &camera_.fov, 1.0f, 120.0f, "%.1f deg");
    ImGui::DragFloat("Near Plane", &camera_.nearPlane, 0.01f, 0.001f,
                     camera_.farPlane - 0.001f, "%.3f");
    ImGui::DragFloat("Far Plane", &camera_.farPlane, 1.0f,
                     camera_.nearPlane + 0.001f, 10000.0f, "%.1f");

    if (camera_.farPlane <= camera_.nearPlane) {
      camera_.farPlane = camera_.nearPlane + 0.001f;
    }

    ImGui::SeparatorText("Input");
    ImGui::Checkbox("Locked", &camera_.locked);
    ImGui::DragFloat("Move Speed", &camera_.movementSpeed, 0.05f, 0.0f,
                     100.0f, "%.2f");
    ImGui::DragFloat("Mouse Sensitivity", &camera_.mouseSensitivity, 0.01f,
                     0.0f, 10.0f, "%.2f");

    if (ImGui::Button("Reset Camera")) {
      camera_.pos = glm::vec3{0.0f, 0.0f, 0.0f};
      camera_.yaw = -90.0f;
      camera_.pitch = 0.0f;
      camera_.fov = 45.0f;
      camera_.nearPlane = 0.1f;
      camera_.farPlane = 1000.0f;
      camera_.firstMouse = true;
      refreshCameraVectors();
    }

    ImGui::SeparatorText("Viewport");
    ImGui::Text("Position: %.1f, %.1f", viewport_info_.x, viewport_info_.y);
    ImGui::Text("Size: %.1f x %.1f", viewport_info_.width,
                viewport_info_.height);
    ImGui::Text("Focused: %s", viewport_info_.isFocused ? "yes" : "no");
    ImGui::Text("Hovered: %s", viewport_info_.isHovered ? "yes" : "no");
  }

  ImGui::End();
}

void UI::renderMeshEditorPanel() {
  auto &meshPanel = panel(DockPanelId::MeshEditor);

  if (ImGui::Begin(meshPanel.name, &meshPanel.open)) {
    const auto meshNames = resource_manager_.listMeshNames();
    const auto selectedMesh = resource_manager_.selectedMeshName();

    ImGui::SeparatorText("Target");

    const char *preview =
        selectedMesh.empty() ? "<none>" : selectedMesh.c_str();
    if (ImGui::BeginCombo("Mesh", preview)) {
      const bool noneSelected = selectedMesh.empty();
      if (ImGui::Selectable("<none>", noneSelected)) {
        resource_manager_.clearSelectedMesh();
      }

      for (const auto &name : meshNames) {
        const bool selected = selectedMesh == name;
        if (ImGui::Selectable(name.c_str(), selected)) {
          resource_manager_.selectMesh(name);
        }

        if (selected) {
          ImGui::SetItemDefaultFocus();
        }
      }

      ImGui::EndCombo();
    }

    if (!selectedMesh.empty()) {
      ImGui::SameLine();
      if (ImGui::Button("Clear")) {
        resource_manager_.clearSelectedMesh();
      }
    }

    ImGui::SeparatorText("Details");
    const auto currentMesh = resource_manager_.selectedMeshName();
    if (currentMesh.empty()) {
      ImGui::TextDisabled("No mesh selected");
    } else {
      ImGui::Text("Name: %s", currentMesh.c_str());

      auto mesh = resource_manager_.getMesh(currentMesh);
      if (mesh && mesh->isValid()) {
        const auto *vertexBuffer = mesh->vertexBufferBase();
        const auto *indexBuffer = mesh->indexBuffer();
        const auto vertexInput = vertexBuffer->vertexInputDesc();

        ImGui::Text("Vertices: %zu", vertexBuffer->vertexCount());
        ImGui::Text("Indices: %zu", indexBuffer->indices().size());
        ImGui::Text("Bindings: %zu", vertexInput.bindings.size());
        ImGui::Text("Attributes: %zu", vertexInput.attributes.size());
      } else {
        ImGui::TextDisabled("State: unavailable");
      }
    }
  }

  ImGui::End();
}

void UI::renderPerformancePanel() {
  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
  auto &performancePanel = panel(DockPanelId::Performance);

  if (ImGui::Begin(performancePanel.name, &performancePanel.open,
                   windowFlags)) {
    if (fps_panel_) {
      fps_panel_->render();
    }
  }

  ImGui::End();
}

void UI::renderShaderEditor() {
  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
  auto &shaderPanel = panel(DockPanelId::ShaderEditor);

  if (ImGui::Begin(shaderPanel.name, &shaderPanel.open, windowFlags)) {
    if (shader_editor_) {
      shader_editor_->render();
    }
  }

  ImGui::End();
}

void UI::renderThemeControls() {
  bool changed = false;

  ImGui::SeparatorText("Accent");

  auto accentItem = [&](const char *label, ThemeAccent accent) {
    if (ImGui::MenuItem(label, nullptr, desc_.accent == accent)) {
      desc_.accent = accent;
      changed = true;
    }
  };

  accentItem("Blue", ThemeAccent::Blue);
  accentItem("Red", ThemeAccent::Red);
  accentItem("Green", ThemeAccent::Green);
  accentItem("Purple", ThemeAccent::Purple);
  accentItem("Amber", ThemeAccent::Amber);

  ImGui::SeparatorText("Mode");

  if (ImGui::MenuItem("Dark", nullptr, desc_.dark)) {
    desc_.dark = true;
    changed = true;
  }

  if (ImGui::MenuItem("Light", nullptr, !desc_.dark)) {
    desc_.dark = false;
    changed = true;
  }

  ImGui::SeparatorText("Shape");
  ImGui::PushItemWidth(140.0f);

  changed |=
      ImGui::SliderFloat("Rounding", &desc_.rounding, 0.0f, 10.0f, "%.1f");
  changed |= ImGui::SliderFloat("Alpha", &desc_.alpha, 0.35f, 1.0f, "%.2f");

  ImGui::PopItemWidth();

  if (changed) {
    Theme::apply(desc_);
  }
}

void UI::renderLoggingPanel() {
  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
  auto &loggingPanel = panel(DockPanelId::Logging);

  if (ImGui::Begin(loggingPanel.name, &loggingPanel.open, windowFlags)) {
    if (logging_panel_) {
      logging_panel_->render();
    }
  }

  ImGui::End();
}

void UI::refreshCameraVectors() {
  camera_.pitch = glm::clamp(camera_.pitch, -89.0f, 89.0f);

  glm::vec3 front{};
  front.x =
      std::cos(glm::radians(camera_.yaw)) * std::cos(glm::radians(camera_.pitch));
  front.y = std::sin(glm::radians(camera_.pitch));
  front.z =
      std::sin(glm::radians(camera_.yaw)) * std::cos(glm::radians(camera_.pitch));

  camera_.front = glm::normalize(front);
  camera_.right = glm::normalize(glm::cross(camera_.front, camera_.worldUp));
  camera_.up = glm::normalize(glm::cross(camera_.right, camera_.front));
  camera_.firstMouse = true;
}

auto UI::panel(DockPanelId id) noexcept -> DockPanelState & {
  return dock_panels_[static_cast<size_t>(id)];
}

auto UI::panel(DockPanelId id) const noexcept -> const DockPanelState & {
  return dock_panels_[static_cast<size_t>(id)];
}

auto UI::panelOpen(DockPanelId id) const noexcept -> bool {
  return panel(id).open;
}

} // namespace vkr::ui
