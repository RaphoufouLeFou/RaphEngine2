#include <RaphEngine2/ui/ui_document.hpp>
#include <RmlUi/Core.h>

namespace raphEngine
{
    static Rml::ElementDocument* Doc(void* p)
    {
        return static_cast<Rml::ElementDocument*>(p);
    }

    bool UIDocument::IsValid() const
    {
        return native_document_ != nullptr;
    }

    void UIDocument::Show()
    {
        if (auto* d = Doc(native_document_))
            d->Show();
    }
    void UIDocument::Hide()
    {
        if (auto* d = Doc(native_document_))
            d->Hide();
    }

    void UIDocument::SetText(const std::string& element_id,
                             const std::string& text)
    {
        if (auto* d = Doc(native_document_))
            if (auto* el = d->GetElementById(element_id))
                el->SetInnerRML(text);
    }

    void UIDocument::SetAttribute(const std::string& element_id,
                                  const std::string& attribute,
                                  const std::string& value)
    {
        if (auto* d = Doc(native_document_))
            if (auto* el = d->GetElementById(element_id))
                el->SetAttribute(attribute, value);
    }

    void UIDocument::SetElementVisible(const std::string& element_id,
                                       bool visible)
    {
        if (auto* d = Doc(native_document_))
            if (auto* el = d->GetElementById(element_id))
                el->SetProperty("display", visible ? "block" : "none");
    }
} // namespace raphEngine