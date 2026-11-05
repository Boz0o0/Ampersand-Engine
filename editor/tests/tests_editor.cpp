#include <gtest/gtest.h>

#include <algorithm>
#include <editor/Project.hpp>
#include <editor/Types.hpp>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

using ampersand::editor::Pointer;
using ampersand::editor::Project;
using ampersand::editor::TypeTable;
using json = nlohmann::ordered_json;

/**
 * @brief Gives each test its own temporary folder, with a project created in
 * it, and helpers to write and read its JSON files.
 */
class ProjectTest : public ::testing::Test {
protected:
    void SetUp() override {
        _root = std::filesystem::temp_directory_path() /
                ("ampersand_editor_" +
                 std::string(::testing::UnitTest::GetInstance()
                                 ->current_test_info()
                                 ->name()));
        std::filesystem::remove_all(_root);
        std::filesystem::create_directories(_root);
        auto created = Project::create(_root, "game");
        if (!created) {
            FAIL() << "Cannot create the project";
        }
        _project = std::move(*created);
    }

    void TearDown() override { std::filesystem::remove_all(_root); }

    /** @brief Creates an empty file at relative, in the game folder. */
    void touch(const std::string& relative) const {
        const std::ofstream file(_root / "game" / relative);
    }

    /** @brief Writes data to relative, in the game folder. */
    void write(const std::string& relative, const json& data) const {
        std::ofstream(_root / "game" / relative) << data.dump();
    }

    /** @brief Reads relative, in the game folder. */
    [[nodiscard]] json read(const std::string& relative) const {
        std::ifstream file(_root / "game" / relative);
        return json::parse(file);
    }

    /** @brief Writes a scene holding entities and opens it. */
    void openScene(const json& entities) {
        write("world/scenes/test.json", {
                                            {"version", 2},
                                            {"resources", json::object()},
                                            {"entities", entities},
                                        });
        ASSERT_TRUE(project().openScene("world/scenes/test.json"));
    }

    /** @brief The project created by SetUp. */
    Project& project() { return _project; }

    /** @brief The opened scene. */
    ampersand::editor::Scene& opened() {
        auto& scene = _project.getScene();
        if (!scene) {
            throw std::logic_error("No scene is opened");
        }
        return *scene;
    }

    /** @brief Data of the opened scene. */
    json& scene() { return opened().document.data; }

    std::filesystem::path _root;
    Project _project;
};

TEST(ProjectNameTest, AcceptsPlainNames) {
    EXPECT_TRUE(Project::isValidName("level1"));
    EXPECT_TRUE(Project::isValidName("My Game"));
    EXPECT_TRUE(Project::isValidName("a.b"));
    EXPECT_TRUE(Project::isValidName("quote\"d"));
}

TEST(ProjectNameTest, RefusesPathsAndEmptyNames) {
    EXPECT_FALSE(Project::isValidName(""));
    EXPECT_FALSE(Project::isValidName("."));
    EXPECT_FALSE(Project::isValidName(".."));
    EXPECT_FALSE(Project::isValidName("../outside"));
    EXPECT_FALSE(Project::isValidName("a/b"));
    EXPECT_FALSE(Project::isValidName("a\\b"));
    EXPECT_FALSE(Project::isValidName("C:"));
    EXPECT_FALSE(Project::isValidName("new\nline"));
}

TEST_F(ProjectTest, CreateWritesTheFolders) {
    EXPECT_TRUE(std::filesystem::is_directory(_root / "game" / "assets"));
    EXPECT_TRUE(std::filesystem::is_directory(_root / "game" / "scripts"));
    EXPECT_TRUE(
        std::filesystem::is_directory(_root / "game" / "world/prefabs"));
    EXPECT_TRUE(std::filesystem::is_directory(_root / "game" / "world/scenes"));
}

TEST_F(ProjectTest, CreateWritesTheConfig) {
    const json config = read("world/config.json");
    EXPECT_EQ(config["version"], 2);
    EXPECT_EQ(config["gameRoot"], "game/");
    EXPECT_EQ(config["startScene"], "defaultScene");
    EXPECT_EQ(config["scenes"]["defaultScene"],
              "world/scenes/defaultScene.json");
    EXPECT_TRUE(config["resources"].is_object());
}

TEST_F(ProjectTest, CreateWritesTheDefaultScene) {
    EXPECT_TRUE(project().openScene("world/scenes/defaultScene.json"));
    EXPECT_EQ(opened().name, "defaultScene");
    EXPECT_FALSE(project().isDirty());
}

TEST_F(ProjectTest, CreateEscapesTheNameInTheConfig) {
    auto project = Project::create(_root, "My \"Game\"");
    ASSERT_TRUE(project.has_value());
    EXPECT_EQ(project.value().getProjectName(), "My \"Game\"");
    EXPECT_EQ(project.value().getConfig().data["gameRoot"], "My \"Game\"/");
}

TEST_F(ProjectTest, CreateRefusesAnInvalidName) {
    EXPECT_FALSE(Project::create(_root, "../outside").has_value());
    EXPECT_FALSE(std::filesystem::exists(_root.parent_path() / "outside"));
}

TEST_F(ProjectTest, OpenRefusesAFolderWithoutConfig) {
    std::filesystem::create_directories(_root / "empty");
    EXPECT_FALSE(Project::open(_root / "empty").has_value());
    EXPECT_FALSE(Project::open(_root / "missing").has_value());
    write("world/config.json", json::array());
    EXPECT_FALSE(Project::open(_root / "game").has_value());
}

TEST_F(ProjectTest, RefreshFilesListsFoldersFirstThenByName) {
    touch("assets/b.png");
    touch("assets/a.png");
    std::filesystem::create_directories(_root / "game" / "assets" / "z");
    project().refreshFiles();
    const auto& files = project().getFiles();
    ASSERT_EQ(files.size(), 3);
    EXPECT_EQ(files[0].name, "assets");
    EXPECT_EQ(files[1].name, "scripts");
    EXPECT_EQ(files[2].name, "world");
    const auto& assets = files[0].children;
    ASSERT_EQ(assets.size(), 3);
    EXPECT_EQ(assets[0].name, "z");
    EXPECT_TRUE(assets[0].directory);
    EXPECT_EQ(assets[1].name, "a.png");
    EXPECT_EQ(assets[2].name, "b.png");
    EXPECT_EQ(assets[1].path, std::filesystem::path("assets") / "a.png");
}

TEST_F(ProjectTest, CreateSceneWritesAnEmptyScene) {
    EXPECT_EQ(project().createScene("level \"1\""), std::nullopt);
    const json scene = read("world/scenes/level \"1\".json");
    EXPECT_EQ(scene["version"], 2);
    EXPECT_EQ(scene["name"], "level \"1\"");
    EXPECT_TRUE(scene["entities"].is_array());
    EXPECT_TRUE(project().openScene("world/scenes/level \"1\".json"));
}

TEST_F(ProjectTest, CreateSceneRefusesInvalidAndExistingNames) {
    EXPECT_NE(project().createScene("../../outside"), std::nullopt);
    EXPECT_FALSE(std::filesystem::exists(_root / "game" / "outside.json"));
    EXPECT_NE(project().createScene("defaultScene"), std::nullopt);
    EXPECT_TRUE(read("world/scenes/defaultScene.json")["entities"].empty());
}

TEST_F(ProjectTest, CreateSceneDoesNotChangeTheConfig) {
    EXPECT_EQ(project().createScene("other"), std::nullopt);
    EXPECT_FALSE(project().getConfig().data["scenes"].contains("other"));
    EXPECT_FALSE(project().isDirty());
}

TEST_F(ProjectTest, OpenSceneRefusesPrefabsAndConfig) {
    write("world/prefabs/enemy.json",
          {{"version", 2}, {"components", json::object()}});
    EXPECT_FALSE(project().openScene("world/prefabs/enemy.json"));
    EXPECT_FALSE(project().openScene("world/config.json"));
    EXPECT_FALSE(project().openScene("world/scenes/missing.json"));
    EXPECT_FALSE(project().getScene().has_value());
}

TEST_F(ProjectTest, OpenSceneUsesTheFileNameWithoutName) {
    openScene(json::array());
    EXPECT_EQ(opened().name, "test");
}

TEST_F(ProjectTest, BuildWritesOnlyTheChangedDocuments) {
    openScene(json::array());
    project().getConfig().data["startScene"] = "test";
    project().getConfig().dirty = true;
    std::filesystem::remove(_root / "game" / "world/scenes/test.json");
    EXPECT_TRUE(project().isDirty());
    EXPECT_FALSE(project().isSceneDirty());
    EXPECT_EQ(project().build(), std::nullopt);
    EXPECT_EQ(read("world/config.json")["startScene"], "test");
    EXPECT_FALSE(
        std::filesystem::exists(_root / "game" / "world/scenes/test.json"));
    EXPECT_FALSE(project().isDirty());
}

TEST_F(ProjectTest, BuildWritesTheChangedScene) {
    openScene(json::array());
    ASSERT_TRUE(project().createEntity(Pointer()));
    EXPECT_TRUE(project().isSceneDirty());
    EXPECT_EQ(project().build(), std::nullopt);
    EXPECT_EQ(read("world/scenes/test.json")["entities"].size(), 1);
    EXPECT_FALSE(project().isDirty());
}

TEST_F(ProjectTest, EntityEditionNeedsAScene) {
    EXPECT_FALSE(project().createEntity(Pointer()));
    EXPECT_FALSE(project().deleteEntity(Pointer("/entities/0")));
    EXPECT_FALSE(project().renameEntity(Pointer("/entities/0"), "a"));
}

TEST_F(ProjectTest, CreateEntityAddsANamedEntityWithATransform) {
    openScene(json::array());
    ASSERT_TRUE(project().createEntity(Pointer()));
    ASSERT_TRUE(project().createEntity(Pointer("/entities/0")));
    const json& entity = scene()["entities"][0];
    EXPECT_EQ(entity["name"], "Entity");
    EXPECT_EQ(entity["components"]["Transform"],
              TypeTable::get().getComponentDefaults("Transform"));
    EXPECT_EQ(entity["children"].size(), 1);
    EXPECT_TRUE(project().isSceneDirty());
}

TEST_F(ProjectTest, CreateEntityRefusesAParentThatIsNotAnEntity) {
    openScene(json::array());
    EXPECT_FALSE(project().createEntity(Pointer("/entities/0")));
    EXPECT_FALSE(project().createEntity(Pointer("/resources")));
    EXPECT_FALSE(project().isSceneDirty());
}

TEST_F(ProjectTest, DuplicateEntityInsertsACopyAfterIt) {
    openScene(json::array({{{"name", "a"}}, {{"name", "b"}}}));
    ASSERT_TRUE(project().duplicateEntity(Pointer("/entities/0")));
    ASSERT_EQ(scene()["entities"].size(), 3);
    EXPECT_EQ(scene()["entities"][1]["name"], "a");
    EXPECT_EQ(scene()["entities"][2]["name"], "b");
}

TEST_F(ProjectTest, DeleteEntityRemovesItAndItsChildren) {
    openScene(json::array({
        {{"name", "a"}, {"children", json::array({{{"name", "c"}}})}},
        {{"name", "b"}},
    }));
    ASSERT_TRUE(project().deleteEntity(Pointer("/entities/0")));
    ASSERT_EQ(scene()["entities"].size(), 1);
    EXPECT_EQ(scene()["entities"][0]["name"], "b");
    EXPECT_FALSE(project().deleteEntity(Pointer("/entities/5")));
}

TEST_F(ProjectTest, MoveEntityChangesItsParent) {
    openScene(json::array({{{"name", "a"}}, {{"name", "b"}}}));
    ASSERT_TRUE(
        project().moveEntity(Pointer("/entities/1"), Pointer("/entities/0")));
    ASSERT_EQ(scene()["entities"].size(), 1);
    EXPECT_EQ(scene()["entities"][0]["children"][0]["name"], "b");
    ASSERT_TRUE(
        project().moveEntity(Pointer("/entities/0/children/0"), Pointer()));
    EXPECT_EQ(scene()["entities"][1]["name"], "b");
}

TEST_F(ProjectTest, MoveEntityRefusesItselfItsChildrenAndItsParent) {
    openScene(json::array(
        {{{"name", "a"}, {"children", json::array({{{"name", "c"}}})}}}));
    EXPECT_FALSE(
        project().moveEntity(Pointer("/entities/0"), Pointer("/entities/0")));
    EXPECT_FALSE(project().moveEntity(Pointer("/entities/0"),
                                      Pointer("/entities/0/children/0")));
    EXPECT_FALSE(project().moveEntity(Pointer("/entities/0/children/0"),
                                      Pointer("/entities/0")));
    EXPECT_FALSE(project().moveEntity(Pointer("/entities/0"), Pointer()));
    EXPECT_FALSE(project().isSceneDirty());
}

TEST_F(ProjectTest, RenameEntitySetsItsName) {
    openScene(json::array({json::object()}));
    ASSERT_TRUE(project().renameEntity(Pointer("/entities/0"), "Player"));
    EXPECT_EQ(scene()["entities"][0]["name"], "Player");
}

TEST(ProjectPointerTest, IsEntityOnlyAcceptsEntitiesInChildren) {
    const json data = {
        {
            "entities",
            json::array({{{"children", json::array({json::object()})}}}),
        },
        {"resources", {{"Score", json::object()}}},
    };
    EXPECT_TRUE(Project::isEntity(data, Pointer("/entities/0")));
    EXPECT_TRUE(Project::isEntity(data, Pointer("/entities/0/children/0")));
    EXPECT_FALSE(Project::isEntity(data, Pointer()));
    EXPECT_FALSE(Project::isEntity(data, Pointer("/entities")));
    EXPECT_FALSE(Project::isEntity(data, Pointer("/entities/1")));
    EXPECT_FALSE(Project::isEntity(data, Pointer("/resources/Score")));
}

TEST(ProjectPointerTest, IsInsideComparesWholeTokens) {
    EXPECT_TRUE(
        Project::isInside(Pointer("/entities/1"), Pointer("/entities/1")));
    EXPECT_TRUE(Project::isInside(Pointer("/entities/1/children/0"),
                                  Pointer("/entities/1")));
    EXPECT_FALSE(
        Project::isInside(Pointer("/entities/10"), Pointer("/entities/1")));
    EXPECT_FALSE(
        Project::isInside(Pointer("/entities"), Pointer("/entities/1")));
}

TEST(ProjectDiffTest, MergeDiffKeepsOnlyTheChanges) {
    const json base = {{"a", 1}, {"b", {{"x", 1}, {"y", 2}}}, {"c", 3}};
    const json target = {{"a", 1}, {"b", {{"x", 1}, {"y", 5}}}, {"d", 4}};
    const json patch = Project::mergeDiff(base, target);
    EXPECT_EQ(patch, json({{"b", {{"y", 5}}}, {"d", 4}, {"c", nullptr}}));
    json patched = base;
    patched.merge_patch(patch);
    EXPECT_EQ(patched, target);
    EXPECT_TRUE(Project::mergeDiff(base, base).empty());
}

TEST_F(ProjectTest, ResolvedMergesThePrefabComponentsAndChildren) {
    write("world/prefabs/enemy.json",
          {
              {"version", 2},
              {"name", "Enemy"},
              {"components", {{"Stats", {{"health", 3}, {"speed", 1}}}}},
              {"children", json::array({{{"name", "Gun"}}})},
          });
    const json entity = {
        {"prefab", "world/prefabs/./enemy.json"},
        {"components", {{"Stats", {{"speed", 2}}}}},
        {"children", json::array({{{"name", "Hat"}}})},
    };
    const json result = project().resolved(entity);
    EXPECT_EQ(result["components"]["Stats"],
              json({{"health", 3}, {"speed", 2}}));
    ASSERT_EQ(result["children"].size(), 2);
    EXPECT_EQ(result["children"][0]["name"], "Gun");
    EXPECT_EQ(result["children"][1]["name"], "Hat");
}

TEST_F(ProjectTest, ResolvedThrowsOnMissingOrRecursivePrefabs) {
    EXPECT_THROW(
        (void)project().resolved({{"prefab", "world/prefabs/missing.json"}}),
        std::runtime_error);
    EXPECT_THROW((void)project().resolved({{"prefab", 3}}), std::runtime_error);
    write("world/prefabs/loop.json",
          {{"version", 2}, {"prefab", "world/prefabs/loop.json"}});
    EXPECT_THROW(
        (void)project().resolved({{"prefab", "world/prefabs/loop.json"}}),
        std::runtime_error);
}

TEST_F(ProjectTest, GetPrefabReadsTheFileOnce) {
    write("world/prefabs/a.json", {{"version", 2}, {"name", "first"}});
    EXPECT_EQ(project().getPrefab("world/prefabs/a.json")["name"], "first");
    write("world/prefabs/a.json", {{"version", 2}, {"name", "second"}});
    EXPECT_EQ(project().getPrefab("world/prefabs/./a.json")["name"], "first");
    EXPECT_TRUE(project().getPrefab("world/prefabs/none.json").is_null());
}

TEST(TypeTableTest, KnowsTheCoreTransform) {
    const json defaults = TypeTable::get().getComponentDefaults("Transform");
    EXPECT_TRUE(defaults.is_object());
    EXPECT_FALSE(defaults.empty());
    EXPECT_TRUE(TypeTable::get().getComponentDefaults("Unknown").empty());
}

TEST_F(ProjectTest, CreateSceneListsTheNewFile) {
    ASSERT_EQ(project().createScene("boss"), std::nullopt);
    const auto& world = project().getFiles().back();
    ASSERT_EQ(world.name, "world");
    const auto scenes = std::ranges::find_if(
        world.children, [](const auto& node) { return node.name == "scenes"; });
    ASSERT_NE(scenes, world.children.end());
    EXPECT_TRUE(std::ranges::any_of(scenes->children, [](const auto& node) {
        return node.name == "boss.json";
    }));
}
