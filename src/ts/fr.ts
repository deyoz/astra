include(ts/macro.ts)

# meta: suite fr

$(defmacro FR_TEMPLATE
    change
{&lt;?xml version='1.0' encoding='utf-8' standalone='no'?&gt;&{#}13;
&lt;TfrxReport Version='4.13.5' DotMatrixReport='False' EngineOptions.DoublePass='True' IniFile='\Software\Fast Reports' PreviewOptions.Buttons='4095' PreviewOptions.Zoom='1' PrintOptions.Printer='Default' PrintOptions.PrintOnSheet='0' ReportOptions.CreateDate='39042,4886116435' ReportOptions.Description.Text='' ReportOptions.LastChange='42479,8334638542' ScriptLanguage='PascalScript' ScriptText.Text='&amp;{#}13;&amp;{#}10;procedure Memo25OnBeforePrint(Sender: TfrxComponent);&amp;{#}13;&amp;{#}10;begin&amp;{#}13;&amp;{#}10;  if &amp;{#}60;passengers.&amp;{#}34;bag_amount&amp;{#}34;&amp;{#}62; &amp;{#}60;&amp;{#}62; '' then&amp;{#}13;&amp;{#}10;    Memo25.Text := &amp;{#}60;passengers.&amp;{#}34;bag_amount&amp;{#}34;&amp;{#}62;+'/'+&amp;{#}60;passengers.&amp;{#}34;bag_weight&amp;{#}34;&amp;{#}62;;&amp;{#}13;&amp;{#}10;end;&amp;{#}13;&amp;{#}10;&amp;{#}13;&amp;{#}10;procedure Memo28OnBeforePrint(Sender: TfrxComponent);&amp;{#}13;&amp;{#}10;begin&amp;{#}13;&amp;{#}10;  if &amp;{#}60;passengers.&amp;{#}34;rk_amount&amp;{#}34;&amp;{#}62; &amp;{#}60;&amp;{#}62; '' then&amp;{#}13;&amp;{#}10;    Memo28.Text := &amp;{#}60;passengers.&amp;{#}34;rk_amount&amp;{#}34;&amp;{#}62;+'/'+&amp;{#}60;passengers.&amp;{#}34;rk_weight&amp;{#}34;&amp;{#}62;;&amp;{#}13;&amp;{#}10;end;&amp;{#}13;&amp;{#}10;&amp;{#}13;&amp;{#}10;procedure Memo29OnBeforePrint(Sender: TfrxComponent);&amp;{#}13;&amp;{#}10;begin&amp;{#}13;&amp;{#}10;  if &amp;{#}60;passengers.&amp;{#}34;excess&amp;{#}34;&amp;{#}62; &amp;{#}60;&amp;{#}62; '' then&amp;{#}13;&amp;{#}10;  begin              &amp;{#}13;&amp;{#}10;    Memo29.Text := &amp;{#}60;passengers.&amp;{#}34;excess&amp;{#}34;&amp;{#}62;;&amp;{#}13;&amp;{#}10;    if &amp;{#}60;passengers.&amp;{#}34;pr_payment&amp;{#}34;&amp;{#}62; &amp;{#}60;&amp;{#}62; '1' then&amp;{#}13;&amp;{#}10;      Memo29.Color := cl3DDkShadow&amp;{#}13;&amp;{#}10;    else                        &amp;{#}13;&amp;{#}10;      Memo29.Color := clNone;  &amp;{#}13;&amp;{#}10;  end&amp;{#}13;&amp;{#}10;  else            &amp;{#}13;&amp;{#}10;      Memo29.Color := clNone;  &amp;{#}13;&amp;{#}10;end;&amp;{#}13;&amp;{#}10;&amp;{#}13;&amp;{#}10;procedure frxReport1OnStartReport(Sender: TfrxComponent);&amp;{#}13;&amp;{#}10;var&amp;{#}13;&amp;{#}10;  i: integer;&amp;{#}13;&amp;{#}10;  str: string;                               &amp;{#}13;&amp;{#}10;begin&amp;{#}13;&amp;{#}10;  if Get('test_server') = '0' then&amp;{#}13;&amp;{#}10;    Page1.BackPicture := nil;&amp;{#}13;&amp;{#}10;  TestMemo.Visible := &amp;{#}60;test_server&amp;{#}62; = '1';&amp;{#}13;&amp;{#}10;  if TestMemo.Visible then&amp;{#}13;&amp;{#}10;  begin&amp;{#}13;&amp;{#}10;    for i := 0 to 50 do            &amp;{#}13;&amp;{#}10;      str := str + &amp;{#}60;doc_cap_test&amp;{#}62; + '   ';&amp;{#}13;&amp;{#}10;    TestMemo.Text := str;                                                          &amp;{#}13;&amp;{#}10;  end;  &amp;{#}13;&amp;{#}10;end;&amp;{#}13;&amp;{#}10;&amp;{#}13;&amp;{#}10;procedure Memo24OnBeforePrint(Sender: TfrxComponent);&amp;{#}13;&amp;{#}10;begin&amp;{#}13;&amp;{#}10;  if &amp;{#}60;passengers.&amp;{#}34;ticket_no&amp;{#}34;&amp;{#}62; &amp;{#}60;&amp;{#}62; '' then&amp;{#}13;&amp;{#}10;  begin&amp;{#}13;&amp;{#}10;    if &amp;{#}60;passengers.&amp;{#}34;coupon_no&amp;{#}34;&amp;{#}62; &amp;{#}60;&amp;{#}62; '' then                                            &amp;{#}13;&amp;{#}10;      Memo24.Text := &amp;{#}60;passengers.&amp;{#}34;ticket_no&amp;{#}34;&amp;{#}62; + '/' + &amp;{#}60;passengers.&amp;{#}34;coupon_no&amp;{#}34;&amp;{#}62;&amp;{#}13;&amp;{#}10;    else                      &amp;{#}13;&amp;{#}10;      Memo24.Text := &amp;{#}60;passengers.&amp;{#}34;ticket_no&amp;{#}34;&amp;{#}62;;&amp;{#}13;&amp;{#}10;  end;            &amp;{#}13;&amp;{#}10;end;&amp;{#}13;&amp;{#}10;&amp;{#}13;&amp;{#}10;procedure Memo15OnBeforePrint(Sender: TfrxComponent);&amp;{#}13;&amp;{#}10;begin&amp;{#}13;&amp;{#}10;  Memo15.Text := Format( &amp;{#}60;page_number_fmt&amp;{#}62;, [ &amp;{#}60;PAGE{#}&amp;{#}62;, &amp;{#}60;TOTALPAGES{#}&amp;{#}62; ] );  &amp;{#}13;&amp;{#}10;end;&amp;{#}13;&amp;{#}10;&amp;{#}13;&amp;{#}10;begin&amp;{#}13;&amp;{#}10;&amp;{#}13;&amp;{#}10;end.' OnStartReport='frxReport1OnStartReport' PropData='044C656674020803546F7002100844617461736574730100095661726961626C65730100055374796C650100'&gt;&{#}13;
  &lt;TfrxDataPage Name='Data' Height='1000' Left='0' Top='0' Width='1000'/&gt;&{#}13;
  &lt;TfrxReportPage Name='Page1' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='0' Orientation='poLandscape' PaperWidth='297' PaperHeight='210' PaperSize='9' LeftMargin='10' RightMargin='10' TopMargin='10' BottomMargin='10' ColumnWidth='0' ColumnPositions.Text='' HGuides.Text='' VGuides.Text=''&gt;&{#}13;
    &lt;TfrxMasterData Name='MasterData1' Height='15,11812' Left='0' Top='170,07885' Width='1046,92981' ColumnWidth='0' ColumnGap='0' DataSetName='passengers' RowCount='0' Stretched='True'&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo8' Left='0' Top='0' Width='30,23624' Height='15,11812' ShowHint='False' StretchMode='smMaxHeight' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='0' Frame.Typ='15' GapX='5' HAlign='haRight' ParentFont='False' Text='[(&amp;{#}60;passengers.&amp;{#}34;reg_no&amp;{#}34;&amp;{#}62;)]'/&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo9' Left='30,23624' Top='0' Width='170,07885' Height='15,11812' ShowHint='False' StretchMode='smMaxHeight' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='0' Frame.Typ='15' GapX='5' ParentFont='False' Text='[(&amp;{#}60;passengers.&amp;{#}34;surname&amp;{#}34;&amp;{#}62;)] [(&amp;{#}60;passengers.&amp;{#}34;name&amp;{#}34;&amp;{#}62;)]'/&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo10' Left='370,39394' Top='0' Width='34,01577' Height='15,11812' ShowHint='False' StretchMode='smMaxHeight' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='0' Frame.Typ='15' GapX='5' HAlign='haCenter' ParentFont='False' Text='[(&amp;{#}60;passengers.&amp;{#}34;pers_type&amp;{#}34;&amp;{#}62;)]'/&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo22' Left='404,40971' Top='0' Width='52,91342' Height='15,11812' ShowHint='False' StretchMode='smMaxHeight' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='0' Frame.Typ='15' GapX='5' HAlign='haRight' ParentFont='False' Text='[(&amp;{#}60;passengers.&amp;{#}34;seat_no&amp;{#}34;&amp;{#}62;)]'/&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo23' Left='457,32313' Top='0' Width='79,37013' Height='15,11812' ShowHint='False' StretchMode='smMaxHeight' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='0' Frame.Typ='15' GapX='5' ParentFont='False' Text='[(&amp;{#}60;passengers.&amp;{#}34;document&amp;{#}34;&amp;{#}62;)]'/&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo24' Left='536,69326' Top='0' Width='98,26778' Height='15,11812' OnBeforePrint='Memo24OnBeforePrint' ShowHint='False' StretchMode='smMaxHeight' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='0' Frame.Typ='15' GapX='5' ParentFont='False' Text=''/&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo25' Left='634,96104' Top='0' Width='49,13389' Height='15,11812' OnBeforePrint='Memo25OnBeforePrint' ShowHint='False' StretchMode='smMaxHeight' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='0' Frame.Typ='15' GapX='5' HAlign='haRight' ParentFont='False' Text=''/&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo26' Left='763,46506' Top='0' Width='185,19697' Height='15,11812' ShowHint='False' StretchMode='smMaxHeight' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='0' Frame.Typ='15' GapX='5' ParentFont='False' Text='[(&amp;{#}60;passengers.&amp;{#}34;tags&amp;{#}34;&amp;{#}62;)]'/&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo28' Left='684,09493' Top='0' Width='45,35436' Height='15,11812' OnBeforePrint='Memo28OnBeforePrint' ShowHint='False' StretchMode='smMaxHeight' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='0' Frame.Typ='15' GapX='5' HAlign='haRight' ParentFont='False' Text=''/&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo29' Left='729,44929' Top='0' Width='34,01577' Height='15,11812' OnBeforePrint='Memo29OnBeforePrint' ShowHint='False' StretchMode='smMaxHeight' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='0' Frame.Typ='15' GapX='5' HAlign='haRight' ParentFont='False' Text=''/&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo27' Left='948,66203' Top='0' Width='98,26778' Height='15,11812' ShowHint='False' StretchMode='smMaxHeight' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='0' Frame.Typ='15' GapX='5' ParentFont='False' Text='[(&amp;{#}60;passengers.&amp;{#}34;remarks&amp;{#}34;&amp;{#}62;)]'/&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo12' Left='200,31509' Top='0' Width='170,07885' Height='15,11812' ShowHint='False' StretchMode='smMaxHeight' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='0' Frame.Typ='15' GapX='5' ParentFont='False' Text='[(&amp;{#}60;passengers.&amp;{#}34;user_descr&amp;{#}34;&amp;{#}62;)]'/&gt;&{#}13;
    &lt;/TfrxMasterData&gt;&{#}13;
    &lt;TfrxPageFooter Name='PageFooter1' Height='52,91342' Left='0' Top='283,46475' Width='1046,92981'&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo1' Left='0' Top='34,01577' Width='718,1107' Height='18,89765' ShowHint='False' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-13' Font.Name='Arial' Font.Style='0' ParentFont='False' VAlign='vaCenter' Text='ISSUE DATE / $(change) [date_issue]'/&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo31' Left='0' Top='0' Width='260,78757' Height='15,11812' ShowHint='False' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='1' GapX='5' ParentFont='False' VAlign='vaCenter' Text='[total]'/&gt;&{#}13;
    &lt;/TfrxPageFooter&gt;&{#}13;
    &lt;TfrxPageHeader Name='PageHeader1' Height='52,91342' Left='0' Top='18,89765' Width='1046,92981'&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo30' Left='0' Top='15,11812' Width='1046,92981' Height='18,89765' ShowHint='False' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-15' Font.Name='Arial' Font.Style='1' HAlign='haCenter' ParentFont='False' Text='[caption]'/&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo15' Left='827,71707' Top='34,01577' Width='219,21274' Height='18,89765' OnBeforePrint='Memo15OnBeforePrint' ShowHint='False' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='1' HAlign='haRight' ParentFont='False' Text='page number'/&gt;&{#}13;
      &lt;TfrxMemoView Name='TestMemo' Left='0' Top='0' Width='1046,92981' Height='15,11812' ShowHint='False' DisplayFormat.DecimalSeparator=',' Font.Charset='1' Font.Color='0' Font.Height='-12' Font.Name='Arial' Font.Style='1' ParentFont='False' VAlign='vaCenter' Text='TEST'/&gt;&{#}13;
    &lt;/TfrxPageHeader&gt;&{#}13;
    &lt;TfrxFooter Name='Footer1' Height='15,11812' Left='0' Top='207,87415' Width='1046,92981' Stretched='True'&gt;&{#}13;
      &lt;TfrxLineView Name='Line1' Left='0' Top='0' Width='1046,92981' Height='0' ShowHint='False' Frame.Typ='4'/&gt;&{#}13;
    &lt;/TfrxFooter&gt;&{#}13;
    &lt;TfrxHeader Name='Header1' Height='15,11812' Left='0' Top='132,28355' Width='1046,92981' ReprintOnNewPage='True'&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo2' Left='0' Top='0' Width='30,23624' Height='15,11812' ShowHint='False' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='1' Frame.Typ='15' GapX='5' ParentFont='False' VAlign='vaCenter' Text='[doc_cap_no]'/&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo3' Left='30,23624' Top='0' Width='170,07885' Height='15,11812' ShowHint='False' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='1' Frame.Typ='15' GapX='5' ParentFont='False' VAlign='vaCenter' Text='[doc_cap_surname]'/&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo7' Left='370,39394' Top='0' Width='34,01577' Height='15,11812' ShowHint='False' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='1' Frame.Typ='15' GapX='5' ParentFont='False' VAlign='vaCenter' Text='[doc_cap_pas]'/&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo18' Left='457,32313' Top='0' Width='79,37013' Height='15,11812' ShowHint='False' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='1' Frame.Typ='15' GapX='5' ParentFont='False' VAlign='vaCenter' Text='[doc_cap_doc]'/&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo19' Left='536,69326' Top='0' Width='98,26778' Height='15,11812' ShowHint='False' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='1' Frame.Typ='15' GapX='5' ParentFont='False' VAlign='vaCenter' Text='[doc_cap_tkt]'/&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo4' Left='634,96104' Top='0' Width='49,13389' Height='15,11812' ShowHint='False' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='1' Frame.Typ='15' GapX='5' ParentFont='False' VAlign='vaCenter' Text='[doc_cap_bag]'/&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo5' Left='684,09493' Top='0' Width='45,35436' Height='15,11812' ShowHint='False' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='1' Frame.Typ='15' GapX='5' ParentFont='False' VAlign='vaCenter' Text='[doc_cap_rk]'/&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo6' Left='729,44929' Top='0' Width='34,01577' Height='15,11812' ShowHint='False' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='1' Frame.Typ='15' GapX='5' ParentFont='False' VAlign='vaCenter' Text='[doc_cap_pay]'/&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo20' Left='763,46506' Top='0' Width='185,19697' Height='15,11812' ShowHint='False' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='1' Frame.Typ='15' GapX='5' ParentFont='False' VAlign='vaCenter' Text='[doc_cap_tags]'/&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo21' Left='948,66203' Top='0' Width='98,26778' Height='15,11812' ShowHint='False' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='1' Frame.Typ='15' GapX='5' ParentFont='False' VAlign='vaCenter' Text='[doc_cap_rem]'/&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo14' Left='404,40971' Top='0' Width='52,91342' Height='15,11812' ShowHint='False' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='a_GroticNrExtraBold' Font.Style='1' Frame.Typ='15' GapX='5' HAlign='haCenter' ParentFont='False' VAlign='vaCenter' Text='[doc_cap_seat_no]'/&gt;&{#}13;
      &lt;TfrxMemoView Name='Memo11' Left='200,31509' Top='0' Width='170,07885' Height='15,11812' ShowHint='False' DisplayFormat.DecimalSeparator='.' Font.Charset='1' Font.Color='0' Font.Height='-11' Font.Name='Arial' Font.Style='1' Frame.Typ='15' GapX='5' ParentFont='False' VAlign='vaCenter' Text='[doc_cap_user_descr]'/&gt;&{#}13;
    &lt;/TfrxHeader&gt;&{#}13;
  &lt;/TfrxReportPage&gt;&{#}13;
&lt;/TfrxReport&gt;&{#}13;})

$(defmacro FR_TEMPLATE_EXPECTED {$(FR_TEMPLATE ë‰Æ‡¨®‡Æ¢†≠Æ)})
$(defmacro FR_TEMPLATE_CHANGED {$(FR_TEMPLATE ë‰Æ‡¨®‡Æ¢†≠)})

$(defmacro CHECK_FR_TEMPLATE
    point_dep
    fr_template
{
!! capture=on
{<?xml version='1.0' encoding='cp866'?>
<term>
  <query handle='0' id='docs' ver='1' opr='PIKE' screen='DOCS.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <run_report2>
      <dev_model/>
      <fmt_type/>
      <prnParams>
        <pr_lat>0</pr_lat>
        <encoding>UTF-16LE</encoding>
        <offset>20</offset>
        <top>0</top>
      </prnParams>
      <point_id>$(point_dep)</point_id>
      <rpt_type>WEB</rpt_type>
      <text>0</text>
      <LoadForm/>
    </run_report2>
  </query>
</term>}

>> lines=4:50
    <form name='web' version='000000-0000000'>$(fr_template)
</form>

}) #end-of-macro

#########################################################################################

$(init_term 201509-0173355)
$(PREPARE_FLIGHT_1PAX_1SEG ûí 103 ÑåÑ èãä êÖèàç àÇÄç)

#$(PREPARE_SEASON_SCD ûí ÑåÑ èãä 103)
#$(make_spp)

$(set point_dep $(last_point_id_spp))

################ í•·‚ SELECT

$(CHECK_FR_TEMPLATE $(get point_dep) $(FR_TEMPLATE_EXPECTED))

################ í•·‚ UPDATE

!! capture=off
{<?xml version='1.0' encoding='cp866'?>
<term>
  <query handle='0' id='docs' ver='1' opr='PIKE' screen='DOCS.EXE' mode='STAND' lang='RU' term_id='2479792165'>
    <save_report>
      <form>$(FR_TEMPLATE_CHANGED)
</form>
      <name>web</name>
    </save_report>
  </query>
</term>}

$(CHECK_FR_TEMPLATE $(get point_dep) $(FR_TEMPLATE_CHANGED))
