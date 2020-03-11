Class Matt.Product Extends %Persistent
{

Property Name As %String;

Storage Default
{
<Data name="ProductDefaultData">
<Value name="1">
<Value>%%CLASSNAME</Value>
</Value>
<Value name="2">
<Value>Name</Value>
</Value>
</Data>
<DataLocation>^Matt.ProductD</DataLocation>
<DefaultData>ProductDefaultData</DefaultData>
<IdLocation>^Matt.ProductD</IdLocation>
<IndexLocation>^Matt.ProductI</IndexLocation>
<StreamLocation>^Matt.ProductS</StreamLocation>
<Type>%Storage.Persistent</Type>
}

}
