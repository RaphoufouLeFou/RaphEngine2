#include <RaphEngine2/ui/ui_document.hpp>
#include <RmlUi/Core.h>

namespace raphEngine
{
    static Rml::ElementDocument* Doc(void* p)
    {
        return static_cast<Rml::ElementDocument*>(p);
    }

    class FunctionEventListener : public Rml::EventListener
    {
    public:
        explicit FunctionEventListener(std::function<void()> callback)
            : callback_(std::move(callback))
        {}

        void ProcessEvent(Rml::Event&) override
        {
            if (callback_)
                callback_();
        }

        void OnDetach(Rml::Element*) override
        {
            delete this;
        }

    private:
        std::function<void()> callback_;
    };

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

    bool UIDocument::IsVisible() const
    {
        auto* d = Doc(native_document_);
        return d && d->IsVisible();
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

    void UIDocument::OnClick(const std::string& element_id,
                             std::function<void()> callback)
    {
        auto* d = Doc(native_document_);
        if (!d)
            return;
        auto* el = d->GetElementById(element_id);
        if (!el)
            return;

        el->AddEventListener("click",
                             new FunctionEventListener(std::move(callback)));
    }
} // namespace raphEngine