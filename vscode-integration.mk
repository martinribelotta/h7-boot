_MK_COMMA:=,
_MK_QUOTE:="
_MK_EMPTY:=
_MK_OBRACK:={
_MK_CBRACK:=}
_MK_IDENT2:=$(_MK_EMPTY)                $(_MK_EMPTY)
_MK_IDENT1:=$(_MK_EMPTY)        $(_MK_EMPTY)
_MK_SPACE:=$(_MK_EMPTY) $(_MK_EMPTY)
_MK_NEWLINE:=\n

define DOT_VSCODE_C_CPP_PROPERTIES
{
    "configurations": [
        {
            "name": "$(TARGET)",
            "includePath": [
$(_MK_IDENT2)$(subst $(_MK_SPACE),$(_MK_COMMA)$(_MK_NEWLINE)$(_MK_IDENT2),$(addsuffix $(_MK_QUOTE), $(addprefix $(_MK_QUOTE)$${workspaceFolder}/, $(C_INCLUDES))))
            ],
            "defines": [
$(_MK_IDENT2)$(subst $(_MK_SPACE),$(_MK_COMMA)$(_MK_NEWLINE)$(_MK_IDENT2),$(addsuffix $(_MK_QUOTE), $(addprefix $(_MK_QUOTE), $(C_DEFS))))
            ],
            "compilerArgs": [
$(_MK_IDENT2)$(subst $(_MK_SPACE),$(_MK_COMMA)$(_MK_NEWLINE)$(_MK_IDENT2),$(addsuffix $(_MK_QUOTE), $(addprefix $(_MK_QUOTE), $(COMMON_FLAGS) $(OPT_FLAGS))))
            ],
            "compilerPath": "$(CC)",
            "cStandard": "c11",
            "cppStandard": "c++11",
            "intelliSenseMode": "gcc-arm"
        }
    ],
    "version": 4
}
endef
export DOT_VSCODE_C_CPP_PROPERTIES

define TASK_TEMPLATE
{
            "problemMatcher": "$$gcc",
            "type": "shell",
            "group": "build",
            "label": "make $1",
            "command": "make $1"
        }
endef

define DOT_VSCODE_TASKS
{
    "version": "2.0.0",
    "tasks": [
$(_MK_IDENT1)$(subst $(_MK_CBRACK) $(_MK_OBRACK),$(_MK_CBRACK)$(_MK_COMMA)\n$(_MK_IDENT1)$(_MK_OBRACK),$(foreach t,$(PHONY_TARGETS),$(call TASK_TEMPLATE,$(t))))
    ]
}
endef
export DOT_VSCODE_TASKS

define DOT_VSCODE_LAUNCH
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Launch $(TARGET)",
            "cwd": "$${workspaceRoot}",
            "executable": "$${workspaceFolder}/$(BUILD_DIR)/$(TARGET).elf",
            "request": "launch",
            "type": "cortex-debug",
            "servertype": "openocd",
            "configFiles": [
                "interface/cmsis-dap.cfg",
                "target/stm32h7x.cfg"
            ],
            "runToMain": true,
            "preLaunchTask": "make all"
        }
    ]
}
endef
export DOT_VSCODE_LAUNCH

.vscode-integration:
	@mkdir -p .vscode
	@echo -n "$$DOT_VSCODE_C_CPP_PROPERTIES" > .vscode/c_cpp_properties.json
	@echo -n "$$DOT_VSCODE_TASKS" > .vscode/tasks.json
	@echo -n "$$DOT_VSCODE_LAUNCH" > .vscode/launch.json
