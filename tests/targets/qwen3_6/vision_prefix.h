#pragma once

#include "ninfer/engine.h"

#include <stdexcept>
#include <string>

namespace ninfer::test {

inline void vision_prefix_reuse(Engine& engine, SpeculativeBackend backend) {
    for (const auto kind : {MediaKind::Image, MediaKind::Video}) {
        const std::string header = "P6\n64 64\n255\n";
        MessagePart media;
        media.kind              = MessagePartKind::Media;
        media.media.kind        = kind;
        media.media.media_type  = "image/x-portable-pixmap";
        media.media.source_name = "pattern.ppm";
        media.media.bytes.assign(header.begin(), header.end());
        for (int i = 0; i < 64 * 64; ++i) {
            media.media.bytes.push_back(i & 255);
            media.media.bytes.push_back((i * 3) & 255);
            media.media.bytes.push_back((i * 7) & 255);
        }
        ChatMessage user;
        user.role = ChatRole::User;
        user.parts.push_back(std::move(media));
        user.parts.push_back(
            {.kind = MessagePartKind::Text, .text = "Describe the pattern briefly.", .media = {}});
        PromptInput input;
        input.messages.push_back(std::move(user));
        input.options.enable_thinking = false;
        RequestOptions options;
        options.execution.requested_output_tokens = 8;
        options.execution.sampling.temperature    = 0.0F;
        options.execution.allow_prefix_reuse      = true;
        options.stop.include_model_defaults       = false;

        const auto first  = engine.generate(engine.prepare(input), options);
        const auto reused = engine.generate(engine.prepare(input), options);
        if (!first.prompt.has_media || first.timings.vision_seconds <= 0 ||
            first.generated_token_ids.size() != 8 ||
            first.finish_reason != FinishReason::OutputLimit ||
            first.speculative.backend != backend ||
            first.speculative.rounds + first.speculative.fallback_steps == 0 ||
            reused.reused_prompt_tokens == 0 || reused.timings.vision_seconds != 0 ||
            reused.finish_reason != FinishReason::OutputLimit ||
            first.generated_token_ids != reused.generated_token_ids) {
            throw std::runtime_error("Vision text-suffix prefix reuse changed the result");
        }
    }
}

} // namespace ninfer::test
