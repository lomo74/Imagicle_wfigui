object Confirmation: TConfirmation
  Left = 0
  Top = 0
  Caption = 'Job submission'
  ClientHeight = 261
  ClientWidth = 409
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object Messages: TMemo
    Left = 0
    Top = 48
    Width = 409
    Height = 179
    Align = alClient
    ReadOnly = True
    ScrollBars = ssBoth
    TabOrder = 0
    ExplicitTop = 41
  end
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 409
    Height = 48
    Align = alTop
    TabOrder = 1
    object Result: TLabel
      Left = 8
      Top = 8
      Width = 52
      Height = 19
      Caption = 'Result'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -16
      Font.Name = 'Tahoma'
      Font.Style = [fsBold]
      ParentFont = False
    end
    object Label2: TLabel
      Left = 8
      Top = 30
      Width = 36
      Height = 13
      Caption = 'Details:'
    end
  end
  object Panel2: TPanel
    Left = 0
    Top = 227
    Width = 409
    Height = 34
    Align = alBottom
    TabOrder = 2
    ExplicitTop = 229
    object Close: TButton
      Left = 152
      Top = 4
      Width = 75
      Height = 25
      Caption = 'Ok'
      Default = True
      ModalResult = 1
      TabOrder = 0
    end
  end
end
