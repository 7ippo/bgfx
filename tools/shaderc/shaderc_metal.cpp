/*
 * Copyright 2011-2025 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "shaderc.h"

#include <iostream> // std::cout

BX_PRAGMA_DIAGNOSTIC_PUSH()
BX_PRAGMA_DIAGNOSTIC_IGNORED_MSVC(4100) // error C4100: 'inclusionDepth' : unreferenced formal parameter
BX_PRAGMA_DIAGNOSTIC_IGNORED_MSVC(4265) // error C4265: 'spv::spirvbin_t': class has virtual functions, but destructor is not virtual
BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC("-Wattributes") // warning: attribute ignored
BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC("-Wdeprecated-declarations") // warning: ‘MSLVertexAttr’ is deprecated
BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC("-Wtype-limits") // warning: comparison of unsigned expression in ‘< 0’ is always false
BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC("-Wshadow") // warning: declaration of 'userData' shadows a member of 'glslang::TShader::Includer::IncludeResult'
#define SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS
#include <spirv_common.hpp>
#include <spirv_msl.hpp>
#include <spirv_reflect.hpp>

#define ENABLE_OPT 1
#include <ShaderLang.h>
#include <ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>
#include <SPIRV/SPVRemapper.h>
#include <SPIRV/SpvTools.h>
#include <spirv-tools/optimizer.hpp>
BX_PRAGMA_DIAGNOSTIC_POP()

namespace bgfx
{
	struct TinyStlAllocator
	{
		static void* static_allocate(size_t _bytes);
		static void static_deallocate(void* _ptr, size_t /*_bytes*/);
	};

} // namespace bgfx

#define TINYSTL_ALLOCATOR bgfx::TinyStlAllocator
#include <tinystl/allocator.h>
#include <tinystl/string.h>
#include <tinystl/unordered_map.h>
#include <tinystl/vector.h>
namespace stl = tinystl;

#include "../../src/shader.h"

namespace bgfx { namespace metal
{
	const TBuiltInResource resourceLimits =
	{
		32,    // MaxLights
		6,     // MaxClipPlanes
		32,    // MaxTextureUnits
		32,    // MaxTextureCoords
		64,    // MaxVertexAttribs
		4096,  // MaxVertexUniformComponents
		64,    // MaxVaryingFloats
		32,    // MaxVertexTextureImageUnits
		80,    // MaxCombinedTextureImageUnits
		32,    // MaxTextureImageUnits
		4096,  // MaxFragmentUniformComponents
		32,    // MaxDrawBuffers
		128,   // MaxVertexUniformVectors
		8,     // MaxVaryingVectors
		16,    // MaxFragmentUniformVectors
		16,    // MaxVertexOutputVectors
		15,    // MaxFragmentInputVectors
		-8,    // MinProgramTexelOffset
		7,     // MaxProgramTexelOffset
		8,     // MaxClipDistances
		65535, // MaxComputeWorkGroupCountX
		65535, // MaxComputeWorkGroupCountY
		65535, // MaxComputeWorkGroupCountZ
		1024,  // MaxComputeWorkGroupSizeX
		1024,  // MaxComputeWorkGroupSizeY
		64,    // MaxComputeWorkGroupSizeZ
		1024,  // MaxComputeUniformComponents
		16,    // MaxComputeTextureImageUnits
		8,     // MaxComputeImageUniforms
		8,     // MaxComputeAtomicCounters
		1,     // MaxComputeAtomicCounterBuffers
		60,    // MaxVaryingComponents
		64,    // MaxVertexOutputComponents
		64,    // MaxGeometryInputComponents
		128,   // MaxGeometryOutputComponents
		128,   // MaxFragmentInputComponents
		8,     // MaxImageUnits
		8,     // MaxCombinedImageUnitsAndFragmentOutputs
		8,     // MaxCombinedShaderOutputResources
		0,     // MaxImageSamples
		0,     // MaxVertexImageUniforms
		0,     // MaxTessControlImageUniforms
		0,     // MaxTessEvaluationImageUniforms
		0,     // MaxGeometryImageUniforms
		8,     // MaxFragmentImageUniforms
		8,     // MaxCombinedImageUniforms
		16,    // MaxGeometryTextureImageUnits
		256,   // MaxGeometryOutputVertices
		1024,  // MaxGeometryTotalOutputComponents
		1024,  // MaxGeometryUniformComponents
		64,    // MaxGeometryVaryingComponents
		128,   // MaxTessControlInputComponents
		128,   // MaxTessControlOutputComponents
		16,    // MaxTessControlTextureImageUnits
		1024,  // MaxTessControlUniformComponents
		4096,  // MaxTessControlTotalOutputComponents
		128,   // MaxTessEvaluationInputComponents
		128,   // MaxTessEvaluationOutputComponents
		16,    // MaxTessEvaluationTextureImageUnits
		1024,  // MaxTessEvaluationUniformComponents
		120,   // MaxTessPatchComponents
		32,    // MaxPatchVertices
		64,    // MaxTessGenLevel
		16,    // MaxViewports
		0,     // MaxVertexAtomicCounters
		0,     // MaxTessControlAtomicCounters
		0,     // MaxTessEvaluationAtomicCounters
		0,     // MaxGeometryAtomicCounters
		8,     // MaxFragmentAtomicCounters
		8,     // MaxCombinedAtomicCounters
		1,     // MaxAtomicCounterBindings
		0,     // MaxVertexAtomicCounterBuffers
		0,     // MaxTessControlAtomicCounterBuffers
		0,     // MaxTessEvaluationAtomicCounterBuffers
		0,     // MaxGeometryAtomicCounterBuffers
		1,     // MaxFragmentAtomicCounterBuffers
		1,     // MaxCombinedAtomicCounterBuffers
		16384, // MaxAtomicCounterBufferSize
		4,     // MaxTransformFeedbackBuffers
		64,    // MaxTransformFeedbackInterleavedComponents
		8,     // MaxCullDistances
		8,     // MaxCombinedClipAndCullDistances
		4,     // MaxSamples
		0,     // maxMeshOutputVerticesNV
		0,     // maxMeshOutputPrimitivesNV
		0,     // maxMeshWorkGroupSizeX_NV
		0,     // maxMeshWorkGroupSizeY_NV
		0,     // maxMeshWorkGroupSizeZ_NV
		0,     // maxTaskWorkGroupSizeX_NV
		0,     // maxTaskWorkGroupSizeY_NV
		0,     // maxTaskWorkGroupSizeZ_NV
		0,     // maxMeshViewCountNV
		0,     // maxMeshOutputVerticesEXT
		0,     // maxMeshOutputPrimitivesEXT
		0,     // maxMeshWorkGroupSizeX_EXT
		0,     // maxMeshWorkGroupSizeY_EXT
		0,     // maxMeshWorkGroupSizeZ_EXT
		0,     // maxTaskWorkGroupSizeX_EXT
		0,     // maxTaskWorkGroupSizeY_EXT
		0,     // maxTaskWorkGroupSizeZ_EXT
		0,     // maxMeshViewCountEXT
		0,     // maxDualSourceDrawBuffersEXT

		{ // limits
			true, // nonInductiveForLoops
			true, // whileLoops
			true, // doWhileLoops
			true, // generalUniformIndexing
			true, // generalAttributeMatrixVectorIndexing
			true, // generalVaryingIndexing
			true, // generalSamplerIndexing
			true, // generalVariableIndexing
			true, // generalConstantMatrixVectorIndexing
		},
	};

	static EShLanguage getLang(char _p)
	{
		switch (_p)
		{
		case 'c': return EShLangCompute;
		case 'f': return EShLangFragment;
		case 'v': return EShLangVertex;
		default:  return EShLangCount;
		}
	}

	static const char* s_attribName[] =
	{
		"a_position",
		"a_normal",
		"a_tangent",
		"a_bitangent",
		"a_color0",
		"a_color1",
		"a_color2",
		"a_color3",
		"a_indices",
		"a_weight",
		"a_texcoord0",
		"a_texcoord1",
		"a_texcoord2",
		"a_texcoord3",
		"a_texcoord4",
		"a_texcoord5",
		"a_texcoord6",
		"a_texcoord7",
	};
	static_assert(bgfx::Attrib::Count == BX_COUNTOF(s_attribName) );

	bgfx::Attrib::Enum toAttribEnum(const bx::StringView& _name)
	{
		for (uint8_t ii = 0; ii < Attrib::Count; ++ii)
		{
			if (0 == bx::strCmp(s_attribName[ii], _name) )
			{
				return bgfx::Attrib::Enum(ii);
			}
		}

		return bgfx::Attrib::Count;
	}

	static const char* s_samplerTypes[] =
	{
		"BgfxSampler2D",
		"BgfxISampler2D",
		"BgfxUSampler2D",
		"BgfxSampler2DArray",
		"BgfxSampler2DShadow",
		"BgfxSampler2DArrayShadow",
		"BgfxSampler3D",
		"BgfxISampler3D",
		"BgfxUSampler3D",
		"BgfxSamplerCube",
		"BgfxSamplerCubeShadow",
		"BgfxSampler2DMS",
	};

	static uint16_t writeUniformArray(bx::WriterI* _shaderWriter, const UniformArray& uniforms, bool isFragmentShader)
	{
		uint16_t size = 0;

		bx::ErrorAssert err;

		uint16_t count = uint16_t(uniforms.size());
		bx::write(_shaderWriter, count, &err);

		uint32_t fragmentBit = isFragmentShader ? kUniformFragmentBit : 0;
		for (uint16_t ii = 0; ii < count; ++ii)
		{
			const Uniform& un = uniforms[ii];

			size += un.regCount*16;

			uint8_t nameSize = (uint8_t)un.name.size();
			bx::write(_shaderWriter, nameSize, &err);
			bx::write(_shaderWriter, un.name.c_str(), nameSize, &err);
			bx::write(_shaderWriter, uint8_t(un.type | fragmentBit), &err);
			bx::write(_shaderWriter, un.num, &err);
			bx::write(_shaderWriter, un.regIndex, &err);
			bx::write(_shaderWriter, un.regCount, &err);
			bx::write(_shaderWriter, un.texComponent, &err);
			bx::write(_shaderWriter, un.texDimension, &err);
			bx::write(_shaderWriter, un.texFormat, &err);

			BX_TRACE("%s, %s, %d, %d, %d"
				, un.name.c_str()
				, getUniformTypeName(un.type)
				, un.num
				, un.regIndex
				, un.regCount
			);
		}
		return size;
	}

	static spv_target_env getSpirvTargetVersion(uint32_t _version, bx::WriterI* _messageWriter)
	{
		bx::ErrorAssert err;

		switch (_version)
		{
		case 1000:
		case 1110:
		case 1210:
			return SPV_ENV_VULKAN_1_0;
		case 2011:
		case 2111:
		case 2211:
			return SPV_ENV_VULKAN_1_1;
		case 2314:
		case 2414:
		case 3014:
		case 3114:
			return SPV_ENV_VULKAN_1_1_SPIRV_1_4;
		default:
			bx::write(_messageWriter, &err, "Warning: Unknown SPIR-V version requested. Returning SPV_ENV_VULKAN_1_0 as default.\n");
			return SPV_ENV_VULKAN_1_0;
		}
	}

	static glslang::EShTargetLanguageVersion getGlslangTargetSpirvVersion(uint32_t _version, bx::WriterI* _messageWriter)
	{
		bx::ErrorAssert err;

		switch (_version)
		{
		case 1000:
		case 1110:
		case 1210:
			return glslang::EShTargetSpv_1_0;
		case 2011:
		case 2111:
		case 2211:
			return glslang::EShTargetSpv_1_1;
		case 2314:
		case 2414:
		case 3014:
		case 3114:
			return glslang::EShTargetSpv_1_4;
		default:
			bx::write(_messageWriter, &err, "Warning: Unknown SPIR-V version requested. Returning EShTargetSpv_1_0 as default.\n");
			return glslang::EShTargetSpv_1_0;
		}
	}

	static spirv_cross::CompilerMSL::Options::Platform getMslPlatform(const std::string& _platform)
	{
		return "ios" == _platform
			? spirv_cross::CompilerMSL::Options::Platform::iOS
			: spirv_cross::CompilerMSL::Options::Platform::macOS;
	}

	static void getMSLVersion(const uint32_t _version, uint32_t& _major, uint32_t& _minor, bx::WriterI* _messageWriter)
	{
		bx::ErrorAssert err;

		_major = _version / 1000;
		_minor = (_version / 100) % 10;

		switch (_version)
		{
		case 1000:
		case 1110:
		case 1210:
		case 2011:
		case 2111:
		case 2211:
		case 2314:
		case 2414:
		case 3014:
		case 3114:
			return;
		default:
			bx::write(_messageWriter, &err, "Warning: Unknown MSL version requested. Returning 1.0 as default.\n");
			_major = 1;
			_minor = 0;
		}
	}

	static bool compile(const Options& _options, uint32_t _version, const std::string& _code, bx::WriterI* _shaderWriter, bx::WriterI* _messageWriter, bool _firstPass)
	{
		BX_UNUSED(_version);

		bx::ErrorAssert messageErr;

		glslang::InitializeProcess();

		EShLanguage stage = getLang(_options.shaderType);
		if (EShLangCount == stage)
		{
			bx::write(_messageWriter, &messageErr, "Error: Unknown shader type '%c'.\n", _options.shaderType);
			return false;
		}

		glslang::TProgram* program = new glslang::TProgram;
		glslang::TShader* shader   = new glslang::TShader(stage);

		EShMessages messages = EShMessages(0
			| EShMsgDefault
			| EShMsgReadHlsl
			| EShMsgVulkanRules
			| EShMsgSpvRules
			| EShMsgDebugInfo
			);

		shader->setEntryPoint("main");
		shader->setAutoMapBindings(true);
		shader->setEnvTarget(glslang::EShTargetSpv, getGlslangTargetSpirvVersion(_version, _messageWriter));
		const int textureBindingOffset = 16;
		shader->setShiftBinding(glslang::EResTexture, textureBindingOffset);
		shader->setShiftBinding(glslang::EResSampler, textureBindingOffset);
		shader->setShiftBinding(glslang::EResImage, textureBindingOffset);

		const char* shaderStrings[] = { _code.c_str() };
		shader->setStrings(
			  shaderStrings
			, BX_COUNTOF(shaderStrings)
			);
		bool compiled = shader->parse(&resourceLimits
			, 110
			, false
			, messages
			);
		bool linked = false;
		bool validated = true;

		if (!compiled)
		{
			const char* log = shader->getInfoLog();
			if (NULL != log)
			{
				int32_t source  = 0;
				int32_t line    = 0;
				int32_t column  = 0;
				int32_t start   = 0;
				int32_t end     = INT32_MAX;

				bx::StringView err = bx::strFind(log, "ERROR:");

				bool found = false;

				if (!err.isEmpty() )
				{
					found = 2 == sscanf(err.getPtr(), "ERROR: %u:%u: '", &source, &line);
					if (found)
					{
						++line;
					}
				}

				if (found)
				{
					start = bx::uint32_imax(1, line-10);
					end   = start + 20;
				}

				printCode(_code.c_str(), bx::uint32_satsub(line, 1), start, end, column);

				bx::write(_messageWriter, &messageErr, "%s\n", log);
			}
		}
		else
		{
			program->addShader(shader);
			linked = true
				&& program->link(messages)
				&& program->mapIO()
				;

			if (!linked)
			{
				const char* log = program->getInfoLog();
				if (NULL != log)
				{
					bx::write(_messageWriter, &messageErr, "%s\n", log);
				}
			}
			else
			{
				program->buildReflection();

				if (_firstPass)
				{
					// first time through, we just find unused uniforms and get rid of them
					std::string output;
					bx::Error err;
					bx::LineReader reader(_code.c_str() );
					while (!reader.isDone() )
					{
						bx::StringView strLine = reader.next();
						bx::StringView str = strFind(strLine, "uniform ");

						if (!str.isEmpty() )
						{
							// If the line declares a uniform, merge all next
							// lines until we encounter a semicolon.
							bx::StringView lineEnd = strFind(strLine, ";");
							while (lineEnd.isEmpty() && !reader.isDone())
							{
								bx::StringView nextLine = reader.next();
								strLine.set(strLine.getPtr(), nextLine.getTerm());
								lineEnd = strFind(nextLine, ";");
							}

							bool found = false;

							for (uint32_t ii = 0; ii < BX_COUNTOF(s_samplerTypes); ++ii)
							{
								if (!bx::findIdentifierMatch(strLine, s_samplerTypes[ii]).isEmpty() )
								{
									found = true;
									break;
								}
							}

							if (!found)
							{
								for (int32_t ii = 0, num = program->getNumLiveUniformVariables(); ii < num; ++ii)
								{
									// matching lines like:  uniform u_name;
									// we want to replace "uniform" with "static" so that it's no longer
									// included in the uniform blob that the application must upload
									// we can't just remove them, because unused functions might still reference
									// them and cause a compile error when they're gone
									if (!bx::findIdentifierMatch(strLine, program->getUniformName(ii) ).isEmpty() )
									{
										found = true;
										break;
									}
								}
							}

							if (!found)
							{
								output.append(strLine.getPtr(), str.getPtr() );
								output += "static ";
								output.append(str.getTerm(), strLine.getTerm() );
								output += "\n";
							}
							else
							{
								output.append(strLine.getPtr(), strLine.getTerm() );
								output += "\n";
							}
						}
						else
						{
							output.append(strLine.getPtr(), strLine.getTerm() );
							output += "\n";
						}
					}

					// recompile with the unused uniforms converted to statics
					delete program;
					delete shader;
					return compile(_options, _version, output.c_str(), _shaderWriter, _messageWriter, false);
				}

				UniformArray uniforms;

				{
					uint16_t count = (uint16_t)program->getNumLiveUniformVariables();

					for (uint16_t ii = 0; ii < count; ++ii)
					{
						Uniform un;
						un.name = program->getUniformName(ii);

						if (bx::hasSuffix(un.name.c_str(), ".@data") )
						{
							continue;
						}

						un.num = uint8_t(program->getUniformArraySize(ii) );
						const uint32_t offset = program->getUniformBufferOffset(ii);
						un.regIndex = uint16_t(offset);
						un.regCount = un.num;

						switch (program->getUniformType(ii) )
						{
						case 0x1404: // GL_INT:
							un.type = UniformType::Sampler;
							break;
						case 0x8B52: // GL_FLOAT_VEC4:
							un.type = UniformType::Vec4;
							break;
						case 0x8B5B: // GL_FLOAT_MAT3:
							un.type = UniformType::Mat3;
							un.regCount *= 3;
							break;
						case 0x8B5C: // GL_FLOAT_MAT4:
							un.type = UniformType::Mat4;
							un.regCount *= 4;
							break;
						default:
							un.type = UniformType::End;
							break;
						}

						uniforms.push_back(un);
					}
				}
				if (g_verbose)
				{
					program->dumpReflection();
				}

				glslang::TIntermediate* intermediate = program->getIntermediate(stage);
				std::vector<uint32_t> spirv;

				glslang::SpvOptions options;
				options.disableOptimizer = _options.debugInformation;
				options.generateDebugInfo = _options.debugInformation;
				options.emitNonSemanticShaderDebugInfo = _options.debugInformation;
				options.emitNonSemanticShaderDebugSource = _options.debugInformation;

				glslang::GlslangToSpv(*intermediate, spirv, &options);

				spvtools::Optimizer opt(getSpirvTargetVersion(_version, _messageWriter));

				auto print_msg_to_stderr = [_messageWriter, &messageErr](
					  spv_message_level_t
					, const char*
					, const spv_position_t&
					, const char* m
					)
				{
					bx::write(_messageWriter, &messageErr, "Error: %s\n", m);
				};

				opt.SetMessageConsumer(print_msg_to_stderr);

				opt.RegisterLegalizationPasses();

				spvtools::ValidatorOptions validatorOptions;
				validatorOptions.SetBeforeHlslLegalization(true);

				if (!opt.Run(
					  spirv.data()
					, spirv.size()
					, &spirv
					, validatorOptions
					, false
					) )
				{
					compiled = false;
				}
				else
				{
					if (g_verbose)
					{
						glslang::SpirvToolsDisassemble(std::cout, spirv, getSpirvTargetVersion(_version, _messageWriter));
					}

					spirv_cross::CompilerReflection refl(spirv);
					spirv_cross::ShaderResources resourcesrefl = refl.get_shader_resources();

					// Loop through the separate_images, and extract the uniform names:
					for (auto& resource : resourcesrefl.separate_images)
					{
						std::string name = refl.get_name(resource.id);
						if (name.size() > 7 && 0 == bx::strCmp(name.c_str() + name.length() - 7, "Texture"))
						{
							name = name.substr(0, name.length() - 7);
						}

						Uniform un;
						un.name = name;
						un.type = UniformType::Sampler;

						un.num = 0;			// needed?
						un.regIndex = 0;	// needed?
						un.regCount = 0;	// needed?

						uniforms.push_back(un);
					}

					uint16_t size = writeUniformArray(_shaderWriter, uniforms, _options.shaderType == 'f');

					bx::Error err;

					spirv_cross::CompilerMSL msl(std::move(spirv) );

					// Configure MSL cross compiler
					spirv_cross::CompilerMSL::Options mslOptions = msl.get_msl_options();
					{
						// - Platform
						mslOptions.platform = getMslPlatform(_options.platform);

						// - MSL Version
						uint32_t major, minor;
						getMSLVersion(_version, major, minor, _messageWriter);
						mslOptions.set_msl_version(major, minor);
					}
					msl.set_msl_options(mslOptions);

					auto executionModel = msl.get_execution_model();
					spirv_cross::MSLResourceBinding newBinding;
					newBinding.stage = executionModel;

					spirv_cross::ShaderResources resources = msl.get_shader_resources();

					spirv_cross::SmallVector<spirv_cross::EntryPoint> entryPoints = msl.get_entry_points_and_stages();
					if (!entryPoints.empty() )
					{
						msl.rename_entry_point(
							entryPoints[0].name
							, "xlatMtlMain"
							, entryPoints[0].execution_model
							);
					}

					for (auto& resource : resources.uniform_buffers)
					{
						unsigned set     = msl.get_decoration(resource.id, spv::DecorationDescriptorSet);
						unsigned binding = msl.get_decoration(resource.id, spv::DecorationBinding);
						newBinding.desc_set   = set;
						newBinding.binding    = binding;
						newBinding.msl_buffer = 0;
						msl.add_msl_resource_binding(newBinding);

						msl.set_name(resource.id, "_mtl_u");
					}

					for (auto& resource : resources.storage_buffers)
					{
						unsigned set     = msl.get_decoration(resource.id, spv::DecorationDescriptorSet);
						unsigned binding = msl.get_decoration(resource.id, spv::DecorationBinding);
						newBinding.desc_set   = set;
						newBinding.binding    = binding;
						newBinding.msl_buffer = binding + 1;
						msl.add_msl_resource_binding(newBinding);
					}

					for (auto& resource : resources.separate_samplers)
					{
						unsigned set     = msl.get_decoration(resource.id, spv::DecorationDescriptorSet);
						unsigned binding = msl.get_decoration(resource.id, spv::DecorationBinding);
						newBinding.desc_set    = set;
						newBinding.binding     = binding;
						newBinding.msl_texture = binding - textureBindingOffset;
						newBinding.msl_sampler = binding - textureBindingOffset;
						msl.add_msl_resource_binding(newBinding);
					}

					for (auto& resource : resources.separate_images)
					{
						std::string name = msl.get_name(resource.id);
						if (name.size() > 7 && 0 == bx::strCmp(name.c_str() + name.length() - 7, "Texture") )
						{
							msl.set_name(resource.id, name.substr(0, name.length() - 7) );
						}

						unsigned set     = msl.get_decoration(resource.id, spv::DecorationDescriptorSet);
						unsigned binding = msl.get_decoration(resource.id, spv::DecorationBinding);
						newBinding.desc_set    = set;
						newBinding.binding     = binding;
						newBinding.msl_texture = binding - textureBindingOffset;
						newBinding.msl_sampler = binding - textureBindingOffset;
						msl.add_msl_resource_binding(newBinding);
					}

					for (auto& resource : resources.storage_images)
					{
						std::string name = msl.get_name(resource.id);

						unsigned set     = msl.get_decoration(resource.id, spv::DecorationDescriptorSet);
						unsigned binding = msl.get_decoration(resource.id, spv::DecorationBinding);
						newBinding.desc_set    = set;
						newBinding.binding     = binding;
						newBinding.msl_texture = binding - textureBindingOffset;
						newBinding.msl_sampler = binding - textureBindingOffset;
						msl.add_msl_resource_binding(newBinding);
					}

					std::string source = msl.compile();

					// fix https://github.com/bkaradzic/bgfx/issues/2822
					// insert struct member which declares point size, defaulted to 1
					if ('v' == _options.shaderType)
					{
						const bx::StringView xlatMtlMainOut("xlatMtlMain_out\n{");
						size_t pos = source.find(xlatMtlMainOut.getPtr() );

						if (pos != std::string::npos)
						{
							pos += xlatMtlMainOut.getLength();
							source.insert(pos, "\n\tfloat bgfx_metal_pointSize [[point_size]] = 1;");
						}
					}

					if ('c' == _options.shaderType)
					{
						for (int i = 0; i < 3; ++i)
						{
							uint16_t dim = (uint16_t)msl.get_execution_mode_argument(
								spv::ExecutionMode::ExecutionModeLocalSize
								, i
								);
							bx::write(_shaderWriter, dim, &err);
						}
					}

					const uint32_t shaderSize = (uint32_t)source.size();
					bx::write(_shaderWriter, shaderSize, &err);
					bx::write(_shaderWriter, source.c_str(), shaderSize, &err);
					const uint8_t nul = 0;
					bx::write(_shaderWriter, nul, &err);

					const uint8_t numAttr = (uint8_t)program->getNumLiveAttributes();
					bx::write(_shaderWriter, numAttr, &err);

					for (uint8_t ii = 0; ii < numAttr; ++ii)
					{
						bgfx::Attrib::Enum attr = toAttribEnum(program->getAttributeName(ii) );
						if (bgfx::Attrib::Count != attr)
						{
							bx::write(_shaderWriter, bgfx::attribToId(attr), &err);
						}
						else
						{
							bx::write(_shaderWriter, uint16_t(UINT16_MAX), &err);
						}
					}

					bx::write(_shaderWriter, size, &err);
				}
			}
		}

		delete program;
		delete shader;

		glslang::FinalizeProcess();

		return compiled && linked && validated;
	}

} // namespace metal

	bool compileMetalShader(const Options& _options, uint32_t _version, const std::string& _code, bx::WriterI* _shaderWriter, bx::WriterI* _messageWriter)
	{
		return metal::compile(_options, _version, _code, _shaderWriter, _messageWriter, true);
	}

} // namespace bgfx
