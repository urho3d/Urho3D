//
// Copyright (c) 2008-2020 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

import org.gradle.internal.io.NullOutputStream
import org.gradle.internal.os.OperatingSystem
import java.time.Duration

plugins {
    id("com.android.library")
    id("com.jfrog.bintray")
    kotlin("android")
    kotlin("android.extensions")
    `maven-publish`
}

android {
    compileSdkVersion(28)
    defaultConfig {
        minSdkVersion(18)
        targetSdkVersion(28)
        versionCode = 1
        versionName = project.version.toString()
        testInstrumentationRunner = "android.support.test.runner.AndroidJUnitRunner"
        externalNativeBuild {
            cmake {
                arguments.apply {
                    System.getenv("ANDROID_CCACHE")?.let { add("-DANDROID_CCACHE=$it") }
                    add("-DGRADLE_BUILD_DIR=$buildDir")
                    // Pass along matching Gradle properties as CMake build options
                    addAll(listOf(
                            "URHO3D_LIB_TYPE",
                            "URHO3D_ANGELSCRIPT",
                            "URHO3D_LUA",
                            "URHO3D_LUAJIT",
                            "URHO3D_LUAJIT_AMALG",
                            "URHO3D_GLES3",
                            "URHO3D_IK",
                            "URHO3D_NETWORK",
                            "URHO3D_PHYSICS",
                            "URHO3D_NAVIGATION",
                            "URHO3D_URHO2D",
                            "URHO3D_PCH",
                            "URHO3D_DATABASE_SQLITE",
                            "URHO3D_WEBP",
                            "URHO3D_FILEWATCHER",
                            "URHO3D_PROFILING",
                            "URHO3D_LOGGING",
                            "URHO3D_THREADING")
                            .filter { project.hasProperty(it) }
                            .map { "-D$it=${project.property(it)}" }
                    )
                    // In order to get clean module segregation, always exclude player/samples from AAR
                    addAll(listOf(
                            "URHO3D_PLAYER",
                            "URHO3D_SAMPLES"
                    ).map { "-D$it=0" })
                }
                targets.add("Urho3D")
            }
        }
        splits {
            abi {
                isEnable = project.hasProperty("ANDROID_ABI")
                reset()
                include(*(project.findProperty("ANDROID_ABI") as String? ?: "")
                        .split(',').toTypedArray())
            }
        }
    }
    buildTypes {
        named("release") {
            isMinifyEnabled = false
            proguardFiles(getDefaultProguardFile("proguard-android.txt"), "proguard-rules.pro")
        }
    }
    externalNativeBuild {
        cmake {
            setVersion(cmakeVersion)
            setPath(project.file("../../CMakeLists.txt"))

            // Make it explicit as one of the task needs to know the exact path and derived from it
            setBuildStagingDirectory(".cxx")
        }
    }
    sourceSets {
        getByName("main") {
            java.srcDir("../../Source/ThirdParty/SDL/android-project/app/src/main/java")
        }
    }
}

dependencies {
    implementation(fileTree(mapOf("dir" to "libs", "include" to listOf("*.jar"))))
    implementation(kotlin("stdlib-jdk8", kotlinVersion))
    implementation("com.getkeepsafe.relinker:relinker:$relinkerVersion")
    testImplementation("junit:junit:$junitVersion")
    androidTestImplementation("com.android.support.test:runner:$testRunnerVersion")
    androidTestImplementation("com.android.support.test.espresso:espresso-core:$testEspressoVersion")
}

lateinit var docABI: String

afterEvaluate {
    // Part of the our external native build tree resided in Gradle buildDir
    // When the buildDir is cleaned then we need a way to re-configure that part back
    // It is achieved by ensuring that CMake configuration phase is rerun
    tasks {
        "clean" {
            doLast {
                android.externalNativeBuild.cmake.path?.touch()
            }
        }
    }

    // This is a hack - workaround Android plugin for Gradle not providing way to bundle extra "stuffs"
    android.buildTypes.forEach { buildType ->
        val config = buildType.name.capitalize()
        tasks {
            register<Zip>("zipBuildTree$config") {
                archiveClassifier.set(buildType.name)
                archiveExtension.set("aar")
                dependsOn("zipBuildTreeConfigurer$config", "bundle${config}Aar")
                from(zipTree(getByName("bundle${config}Aar").outputs.files.first()))
            }
            register<Task>("zipBuildTreeConfigurer$config") {
                val externalNativeBuildDir = File(buildDir, "tree/$config")
                doLast {
                    val zipTask = getByName<Zip>("zipBuildTree$config")
                    externalNativeBuildDir.list()?.forEach { abi ->
                        listOf("include", "lib").forEach {
                            zipTask.from(File(externalNativeBuildDir, "$abi/$it")) {
                                into("tree/$config/$abi/$it")
                            }
                        }
                        if (config == "Release") {
                            docABI = abi
                        }
                    }
                }
            }
            if (System.getenv("CI") != null) {
                "externalNativeBuild$config" {
                    @Suppress("UnstableApiUsage")
                    timeout.set(Duration.ofMinutes(25))
                }
            }
        }
    }
}

tasks {
    register<Jar>("sourcesJar") {
        archiveClassifier.set("sources")
        from(android.sourceSets.getByName("main").java.srcDirs)
    }
    register<Exec>("makeDoc") {
        // Ignore the exit status on Windows host system because Doxygen may not return exit status correctly on Windows
        isIgnoreExitValue = OperatingSystem.current().isWindows
        standardOutput = NullOutputStream.INSTANCE
        args("--build", ".", "--target", "doc")
        dependsOn("makeDocConfigurer")
        mustRunAfter("zipBuildTreeRelease")
    }
    register<Zip>("documentationZip") {
        archiveClassifier.set("documentation")
        dependsOn("makeDoc")
    }
    register<Task>("makeDocConfigurer") {
        doLast {
            val buildTree = File(android.externalNativeBuild.cmake.buildStagingDirectory, "cmake/release/$docABI")
            named<Exec>("makeDoc") {
                // This is a hack - expect the first line to contain the path to the CMake executable
                executable = File(buildTree, "build_command.txt").readLines().first().split(":").last().trim()
                workingDir = buildTree
            }
            named<Zip>("documentationZip") {
                from(File(buildTree, "Docs/html")) {
                    into("docs")
                }
            }
        }
    }
}

publishing {
    publications {
        register<MavenPublication>("mavenAndroid") {
            artifactId = "${project.name}-${project.libraryType}"
            if (project.hasProperty("ANDROID_ABI")) {
                artifactId = "$artifactId-${(project.property("ANDROID_ABI") as String).replace(',', '-')}"
            }
            afterEvaluate {
                // Exclude publishing STATIC-debug AAR because its size exceeds 250MB limit allowed by Bintray
                android.buildTypes
                        .map { it.name }
                        .filter { System.getenv("CI") == null || project.libraryType == "SHARED" || it == "release" }
                        .forEach { artifact(tasks["zipBuildTree${it.capitalize()}"]) }
            }
            artifact(tasks["sourcesJar"])
            artifact(tasks["documentationZip"])
            pom {
                @Suppress("UnstableApiUsage")
                inceptionYear.set("2008")
                @Suppress("UnstableApiUsage")
                licenses {
                    license {
                        name.set("MIT License")
                        url.set("https://github.com/urho3d/Urho3D/blob/master/LICENSE")
                    }
                }
                @Suppress("UnstableApiUsage")
                developers {
                    developer {
                        name.set("Urho3D contributors")
                        url.set("https://github.com/urho3d/Urho3D/graphs/contributors")
                    }
                }
                @Suppress("UnstableApiUsage")
                scm {
                    url.set("https://github.com/urho3d/Urho3D.git")
                    connection.set("scm:git:ssh://git@github.com:urho3d/Urho3D.git")
                    developerConnection.set("scm:git:ssh://git@github.com:urho3d/Urho3D.git")
                }
                withXml {
                    asNode().apply {
                        appendNode("name", "Urho3D")
                        appendNode("description", project.description)
                        appendNode("url", "https://urho3d.github.io/")
                    }
                }
            }
        }
    }
}

bintray {
    user = System.getenv("BINTRAY_USER")
    key = System.getenv("BINTRAY_KEY")
    publish = true
    override = true
    setPublications("mavenAndroid")
    pkg.apply {
        repo = "maven"
        name = project.name
        setLicenses("MIT")
        vcsUrl = "https://github.com/urho3d/Urho3D.git"
        userOrg = "urho3d"
        setLabels("android", "game-development", "game-engine", "open-source", "urho3d")
        websiteUrl = "https://urho3d.github.io/"
        issueTrackerUrl = "https://github.com/urho3d/Urho3D/issues"
        githubRepo = "urho3d/Urho3D"
        publicDownloadNumbers = true
        desc = project.description
        version.apply {
            name = project.version.toString()
            desc = "Continuous delivery from Travis-CI."
        }
    }
}

val Project.libraryType: String
    get() = findProperty("URHO3D_LIB_TYPE") as String? ?: "STATIC"
