#include "pch.h"
#include "AirDribbleIndicator.h"

void AirDribbleIndicator::RenderSettings()
{
    ImGui::TextUnformatted(
        "A plugin to see where to hit the ball to keep it in the air longer "
        "and to visualize your contact points on the ball."
    );
    ImGui::Separator();

    // Optional: force re-init / quick test
    if (ImGui::Button("Render ring indicator"))
    {
        gameWrapper->Execute([this](GameWrapper*)
            {
                cvarManager->executeCommand("Initialize");
            });
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Renders the ring on the ball using the current settings.");

    ImGui::Separator();

    // Lambdas ONLY for small de-duplication, all scoped to this function
    auto sliderFloatCvar = [&](const char* cvarName,
        const char* label,
        float minVal,
        float maxVal,
        const char* tooltip)
        {
            CVarWrapper cvar = cvarManager->getCvar(cvarName);
            if (!cvar) return;

            float value = cvar.getFloatValue();
            if (ImGui::SliderFloat(label, &value, minVal, maxVal))
                cvar.setValue(value);

            if (tooltip && ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", tooltip);
        };

    auto sliderIntCvar = [&](const char* cvarName,
        const char* label,
        int minVal,
        int maxVal,
        const char* tooltip)
        {
            CVarWrapper cvar = cvarManager->getCvar(cvarName);
            if (!cvar) return;

            int value = cvar.getIntValue();
            if (ImGui::SliderInt(label, &value, minVal, maxVal))
                cvar.setValue(value);

            if (tooltip && ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", tooltip);
        };

    // =====================================================
    //                     Ring settings
    // =====================================================
    ImGui::TextUnformatted("Ring settings");
    ImGui::Indent();

    // Enable ring
    {
        CVarWrapper enableCvar = cvarManager->getCvar("ring_enabled");
        if (enableCvar)
        {
            bool enabled = enableCvar.getBoolValue();
            if (ImGui::Checkbox("Enable ring", &enabled))
                enableCvar.setValue(enabled);

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Toggle ring rendering on/off.");
        }
    }

    // Ring distance
    sliderFloatCvar(
        "ring_size",
        "Distance",
        10.0f,
        80.0f,
        "Vertical distance from the bottom of the ball. Default: 15."
    );

    // Ring color
    {
        CVarWrapper colorCvar = cvarManager->getCvar("ring_color");
        if (colorCvar)
        {
            LinearColor color = colorCvar.getColorValue() / 255.0f;
            if (ImGui::ColorEdit4("Ring color",
                &color.R,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar))
            {
                colorCvar.setValue(color * 255.0f);
            }
        }
    }

    ImGui::Unindent();
    ImGui::Separator();

    // =====================================================
    //                  Touch marker settings
    // =====================================================
    ImGui::TextUnformatted("Touch marker settings");
    ImGui::Indent();

    // Enable hit markers
    {
        CVarWrapper enableHitCvar = cvarManager->getCvar("touch_marker_enabled");
        if (enableHitCvar)
        {
            bool enabled = enableHitCvar.getBoolValue();
            if (ImGui::Checkbox("Enable hit markers", &enabled))
                enableHitCvar.setValue(enabled);

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Toggle drawing of touch markers on/off.");
        }
    }

    // Marker size
    sliderFloatCvar(
        "touch_marker_size",
        "Size",
        2.0f,
        15.0f,
        "Marker size in pixels. Default: 8."
    );

    // Marker duration
    sliderFloatCvar(
        "touch_marker_duration",
        "Duration",
        0.5f,
        5.0f,
        "Time (in seconds) each marker remains visible. Default: 2."
    );

    // Max markers
    sliderIntCvar(
        "max_touch_markers",
        "Max markers",
        1,
        10,
        "Maximum number of touch markers rendered at the same time."
    );

    // Correct hit color
    {
        CVarWrapper cvar = cvarManager->getCvar("touch_marker_correct_color");
        if (cvar)
        {
            LinearColor col = cvar.getColorValue() / 255.0f;
            if (ImGui::ColorEdit4("Correct hit color",
                &col.R,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar))
            {
                cvar.setValue(col * 255.0f);
            }
        }
    }

    // Incorrect hit color
    {
        CVarWrapper cvar = cvarManager->getCvar("touch_marker_incorrect_color");
        if (cvar)
        {
            LinearColor col = cvar.getColorValue() / 255.0f;
            if (ImGui::ColorEdit4("Incorrect hit color",
                &col.R,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar))
            {
                cvar.setValue(col * 255.0f);
            }
        }
    }

    // Flip reset hit color
    {
        CVarWrapper cvar = cvarManager->getCvar("touch_marker_reset_color");
        if (cvar)
        {
            LinearColor col = cvar.getColorValue() / 255.0f;
            if (ImGui::ColorEdit4("Flip reset hit color",
                &col.R,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar))
            {
                cvar.setValue(col * 255.0f);
            }
        }
    }

    ImGui::Unindent();
}
