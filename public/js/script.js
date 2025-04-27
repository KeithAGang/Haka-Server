async function fetchDataAndDisplay() {
    const jsonDiv = document.getElementById('json');
    if (!jsonDiv) {
        console.error('Div with id "json" not found.');
        return;
    }

    // Clear the div before fetching new data
    jsonDiv.textContent = '';

    // Clear the div and show a loading message
    jsonDiv.textContent = 'Loading...';

    try {
        // Wait for 3 seconds to simulate a loading animation
        await new Promise(resolve => setTimeout(resolve, 3000));

        // Fetch data from the server
        const response = await fetch('/json');
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        const data = await response.json();

        // Display the fetched data
        jsonDiv.textContent = JSON.stringify(data, null, 2);
    } catch (error) {
        console.error('Error fetching data:', error);
        jsonDiv.textContent = 'Error loading data.';
    }
}
