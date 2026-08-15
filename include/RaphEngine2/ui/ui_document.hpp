#pragma once
#include <RaphEngine2/export.hpp>

#include <string>

namespace raphEngine
{
    class RAPHENGINE_API UIDocument
    {
    public:
        UIDocument() = default;

        bool IsValid() const;

        void Show();
        void Hide();

        void SetText(const std::string& element_id, const std::string& text);
        void SetAttribute(const std::string& element_id,
                          const std::string& attribute,
                          const std::string& value);
        void SetElementVisible(const std::string& element_id, bool visible);

    private:
        friend class UI;
        explicit UIDocument(void* native_document)
            : native_document_(native_document)
        {}
        void* native_document_ = nullptr;
    };
} // namespace raphEngine