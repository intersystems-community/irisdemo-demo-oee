Class Matt.Order Extends (%Persistent, %Populate)
{

Property Product As Product;

Property OrderDate As %Date;

Property Qty As %Integer(MAXVAL = 10, MINVAL = 1) [ Required ];

Property Customer As Customer;

ClassMethod PopulateOrders() As %Status
{
    Set tSC = $$$OK

    Try 
    {       
        Do ##class(Product).%DeleteExtent() 
        Do ##class(Customer).%DeleteExtent() 
        Do ##class(Order).%DeleteExtent() 
    
        For i=1:1:100
        {
            Set product=##class(Product).%New()
            Set product.Name = "Product "_i
            Set tSC = product.%Save()
            Throw:$$$ISERR(tSC) ##class(%Exception.StatusException).CreateFromStatus(tSC)
        }

        Do ##class(Customer).Populate(1000, 1)

        Do ..Populate(10000, 1)
    } 
    Catch (ex)
    {
        Set tSC = ex.AsStatus()
    }

    Quit tSC
}

Storage Default
{
<Data name="OrderDefaultData">
<Value name="1">
<Value>%%CLASSNAME</Value>
</Value>
<Value name="2">
<Value>Product</Value>
</Value>
<Value name="3">
<Value>OrderDate</Value>
</Value>
<Value name="4">
<Value>Qty</Value>
</Value>
<Value name="5">
<Value>Customer</Value>
</Value>
</Data>
<DataLocation>^Matt.OrderD</DataLocation>
<DefaultData>OrderDefaultData</DefaultData>
<IdLocation>^Matt.OrderD</IdLocation>
<IndexLocation>^Matt.OrderI</IndexLocation>
<StreamLocation>^Matt.OrderS</StreamLocation>
<Type>%Storage.Persistent</Type>
}

}
