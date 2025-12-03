#include "UI/Programs/MemoryDefragWizardProgram.h"
#include "UI/ScreenProgramMacros.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

REGISTER_SCREEN_PROGRAM(FMemoryDefragWizardProgram, "MemoryDefragWizard")

FMemoryDefragWizardProgram::FMemoryDefragWizardProgram() { InitializeBlocks(); }

void FMemoryDefragWizardProgram::InitializeBlocks()
{
    Pegs.SetNum(3);
    for (auto& P : Pegs) P.Empty();
    for (int32 i = NumBlocks; i >= 1; --i) Pegs[0].Add(i);
}

bool FMemoryDefragWizardProgram::CanMoveBlock(int32 FromPeg, int32 ToPeg) const
{
    if (FromPeg < 0 || FromPeg >= 3 || ToPeg < 0 || ToPeg >= 3) return false;
    if (FromPeg == ToPeg) return false;
    if (Pegs[FromPeg].Num() == 0) return false;
    if (Pegs[ToPeg].Num() == 0) return true;
    return Pegs[FromPeg].Last() < Pegs[ToPeg].Last();
}

void FMemoryDefragWizardProgram::MoveBlock(int32 FromPeg, int32 ToPeg)
{
    if (!CanMoveBlock(FromPeg, ToPeg)) return;
    Pegs[ToPeg].Add(Pegs[FromPeg].Pop());
    MoveCount++;
    if (CheckWinCondition()) SetTaskComplete(true);
}

bool FMemoryDefragWizardProgram::CheckWinCondition() const { return Pegs[2].Num() == NumBlocks; }

int32 FMemoryDefragWizardProgram::GetPegAtPosition(const FVector2D& Position) const
{
    const FVector2D Size = GetProgramSize();
    const float PegWidth = Size.X / 3.0f;
    const float Margin = PegWidth * 0.07f; // Slight gap so hitboxes align with visual rails
    for (int32 i = 0; i < 3; ++i)
    {
        const float Start = PegWidth * i + Margin;
        const float End = PegWidth * (i + 1) - Margin;
        if (Position.X >= Start && Position.X <= End)
        {
            return i;
        }
    }
    return -1;
}

FVector2D FMemoryDefragWizardProgram::GetPegPosition(int32 PegIndex) const
{
    const FVector2D Size = GetProgramSize();
    float PegWidth = Size.X / 3.0f;
    return FVector2D(PegWidth * PegIndex + PegWidth * 0.5f, Size.Y * 0.6f);
}

FString FMemoryDefragWizardProgram::RenderPegColumn(int32 PegIndex) const
{
    // Render a single peg as vertical ASCII art with blocks stacked bottom-up
    // Max block width is NumBlocks*2 + 2 for brackets
    const int32 MaxBlockWidth = NumBlocks * 2 + 2;
    const int32 RailHeight = NumBlocks + 2; // Space for all blocks plus base
    
    TArray<FString> Lines;
    
    // Peg header
    FString Header = FString::Printf(TEXT("DIMM %c"), TEXT('A') + PegIndex);
    int32 HeaderPad = (MaxBlockWidth - Header.Len()) / 2;
    Lines.Add(FString::ChrN(HeaderPad, TEXT(' ')) + Header);
    
    // Empty rows for blocks not present (top of stack)
    int32 EmptyRows = NumBlocks - Pegs[PegIndex].Num();
    FString Rail = TEXT("|");
    int32 RailPad = MaxBlockWidth / 2;
    FString RailLine = FString::ChrN(RailPad, TEXT(' ')) + Rail;
    for (int32 i = 0; i < EmptyRows; ++i)
    {
        Lines.Add(RailLine);
    }
    
    // Blocks from top to bottom (array is bottom-up, so reverse iteration)
    for (int32 i = Pegs[PegIndex].Num() - 1; i >= 0; --i)
    {
        int32 BlockSize = Pegs[PegIndex][i];
        int32 BlockWidth = BlockSize * 2;
        FString Block = TEXT("[") + FString::ChrN(BlockWidth, TEXT('=')) + TEXT("]");
        int32 BlockPad = (MaxBlockWidth - Block.Len()) / 2;
        Lines.Add(FString::ChrN(BlockPad, TEXT(' ')) + Block);
    }
    
    // Base platform
    FString Base = FString::ChrN(MaxBlockWidth, TEXT('_'));
    Lines.Add(Base);
    
    FString Result;
    for (int32 i = 0; i < Lines.Num(); ++i)
    {
        // Pad all lines to same width
        while (Lines[i].Len() < MaxBlockWidth)
        {
            Lines[i] += TEXT(" ");
        }
        Result += Lines[i];
        if (i < Lines.Num() - 1) Result += TEXT("\n");
    }
    return Result;
}

TSharedRef<SWidget> FMemoryDefragWizardProgram::BuildProgramWidget()
{
    const FScreenProgramStyle& Style = GetProgramStyle();
    const int32 LargeTextSize = Style.TextSize * 2;
    
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(Style.GetSmallPadding())
        [SNew(STextBlock).Text(FText::FromString(TEXT("MEMORY DEFRAG WIZARD")))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", LargeTextSize)).ColorAndOpacity(Style.GetPrimaryColor())]
        + SVerticalBox::Slot().AutoHeight().Padding(0, Style.GetSmallPadding())
        [SNew(STextBlock).Text_Lambda([this]() { 
            if (bShowingReindex) return FText::FromString(TEXT("Reindexing..."));
            return FText::FromString(FString::Printf(TEXT("Moves: %d | Move all blocks to DIMM C"), MoveCount)); 
        }).Font(FCoreStyle::GetDefaultFontStyle("Regular", Style.TextSize)).ColorAndOpacity(Style.GetDimColor())]
        + SVerticalBox::Slot().FillHeight(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
        [SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center)
            [SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(RenderPegColumn(0)); })
                .Font(FCoreStyle::GetDefaultFontStyle("Mono", GetProgramStyle().TextSize * 2))
                .ColorAndOpacity_Lambda([this]() { return (HoveredPeg == 0 || DragSourcePeg == 0) ? GetProgramStyle().GetPrimaryColor() : GetProgramStyle().GetDimColor(); })
                .Justification(ETextJustify::Center)]
            + SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center)
            [SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(RenderPegColumn(1)); })
                .Font(FCoreStyle::GetDefaultFontStyle("Mono", GetProgramStyle().TextSize * 2))
                .ColorAndOpacity_Lambda([this]() { return (HoveredPeg == 1 || DragSourcePeg == 1) ? GetProgramStyle().GetPrimaryColor() : GetProgramStyle().GetDimColor(); })
                .Justification(ETextJustify::Center)]
            + SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Center)
            [SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(RenderPegColumn(2)); })
                .Font(FCoreStyle::GetDefaultFontStyle("Mono", GetProgramStyle().TextSize * 2))
                .ColorAndOpacity_Lambda([this]() { return (HoveredPeg == 2 || DragSourcePeg == 2) ? GetProgramStyle().GetPrimaryColor() : GetProgramStyle().GetDimColor(); })
                .Justification(ETextJustify::Center)]]
        + SVerticalBox::Slot().AutoHeight().Padding(Style.GetSmallPadding())
        [SNew(STextBlock).Text_Lambda([this]() {
            if (DraggedBlock > 0)
            {
                FString Block = FString::ChrN(DraggedBlock * 2, TEXT('='));
                return FText::FromString(FString::Printf(TEXT("Holding: [%s] - Click target DIMM"), *Block));
            }
            return FText::FromString(TEXT("Click a block to pick up"));
        }).Font(FCoreStyle::GetDefaultFontStyle("Regular", Style.TextSize)).ColorAndOpacity(Style.GetDimColor()).Justification(ETextJustify::Center)];
}

void FMemoryDefragWizardProgram::HandlePointerMoved(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor) return;
    DragPosition = Event.ProgramPixelPosition;
    HoveredPeg = GetPegAtPosition(Event.ProgramPixelPosition);
    bool bCanInteract = (DraggedBlock > 0) || (SelectedPeg >= 0) || (HoveredPeg >= 0 && Pegs[HoveredPeg].Num() > 0);
    UpdateCursorForState(DraggedBlock > 0, bCanInteract);
}

void FMemoryDefragWizardProgram::HandlePointerPressed(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    if (bHandledByPreProcessor) return;
    ActivePointerKey = Event.TriggerKey;
    bShowingReindex = false;
    
    int32 ClickedPeg = GetPegAtPosition(Event.ProgramPixelPosition);
    if (ClickedPeg < 0) return;
    
    if (DraggedBlock > 0)
    {
        // Try to place the dragged block
        bool bCanPlace = (Pegs[ClickedPeg].Num() == 0) || (DraggedBlock < Pegs[ClickedPeg].Last());
        if (bCanPlace && ClickedPeg != DragSourcePeg)
        {
            Pegs[ClickedPeg].Add(DraggedBlock);
            MoveCount++;
            DraggedBlock = -1;
            DragSourcePeg = -1;
            if (CheckWinCondition()) SetTaskComplete(true);
        }
        else
        {
            // Invalid drop - return block and show reindex
            Pegs[DragSourcePeg].Add(DraggedBlock);
            DraggedBlock = -1;
            DragSourcePeg = -1;
            bShowingReindex = true;
        }
    }
    else if (SelectedPeg >= 0)
    {
        // Have selection, try to move
        if (CanMoveBlock(SelectedPeg, ClickedPeg))
        {
            MoveBlock(SelectedPeg, ClickedPeg);
            SelectedPeg = -1;
        }
        else if (ClickedPeg == SelectedPeg)
        {
            SelectedPeg = -1; // Deselect
        }
        else
        {
            bShowingReindex = true;
            SelectedPeg = -1;
        }
    }
    else
    {
        // No selection - pick up from clicked peg
        if (Pegs[ClickedPeg].Num() > 0)
        {
            DraggedBlock = Pegs[ClickedPeg].Pop();
            DragSourcePeg = ClickedPeg;
            DragPosition = Event.ProgramPixelPosition;
        }
    }
}

void FMemoryDefragWizardProgram::HandlePointerReleased(const FScreenPointerEvent& Event, bool bHandledByPreProcessor)
{
    ActivePointerKey = EKeys::Invalid;
}
