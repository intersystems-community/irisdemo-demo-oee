Class Matt.Customer Extends (%Persistent, %Populate)
{

Property Name As %String [ Required ];

Property Sex As %String(VALUELIST = ",M,F") [ Required ];

Property City As %String [ Required ];

Storage Default
{
<Data name="CustomerDefaultData">
<Value name="1">
<Value>%%CLASSNAME</Value>
</Value>
<Value name="2">
<Value>Name</Value>
</Value>
<Value name="3">
<Value>Sex</Value>
</Value>
<Value name="4">
<Value>City</Value>
</Value>
</Data>
<DataLocation>^Matt.CustomerD</DataLocation>
<DefaultData>CustomerDefaultData</DefaultData>
<IdLocation>^Matt.CustomerD</IdLocation>
<IndexLocation>^Matt.CustomerI</IndexLocation>
<StreamLocation>^Matt.CustomerS</StreamLocation>
<Type>%Storage.Persistent</Type>
}

}
